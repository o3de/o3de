"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import argparse
import boto3
import hashlib
import json
import math
import netaddr
import time
import urllib2

# Order of operations
#   1) Get list of internal IPs from dogfish
#   2) Translate IP ranges into WAF-vaild CIDR Subnets
#   3) Get InternalIPWhitelist rule from WAF
#   4) Get all list of all IP Sets on that rule
#   5) Loop through all IP ranges in all IP sets
#       a) If the IP address is not in the set of WAF-valid CIDR Subnets, remove it from the IPSet lists
#       b) If the IP address is in the set of WAF-valid CIDR Subnets, remove it from the WAF-valid CIDR subnet list
#   6) Anything left in the WAF-valid CIDR Subnet list needs to be added to IP Sets
#       a) create a new IP set if necessary (max 1000 ranges per IP Set)
#   7) Push updates to WAF

rule_name = "Internal_IP_Whitelist"
ip_set_basename = "Amazon_Internal_IPs"

# Maximum number of IP descriptors per IP Set
max_ranges_per_ip_set = 1000

# Maximum number of IP descriptors updates per call
max_ranges_per_update = 1000

ip_version = "IPV4"


def parse_args():
    """Handle argument parsing and validate destination folder exists before returning argparse args"""
    parser = argparse.ArgumentParser(description='Use AWS CLI to update the Internal IP Whitelist in dev or prod')

    parser.add_argument('-l', '--list', action='store_true',
                        help="List the WAF Valid IPs that the list of IPs translates to instead of running the update.")
    parser.add_argument('-p', '--profile', default='default', help="The name of the AWS CLI profile to use.")

    args = parser.parse_args()
    return args


def translate_and_apply_to_waf(ip_ranges, args):
    subnet_list = list()
    for ip_range in ip_ranges:
        subnet_list.extend(translate_range_to_subnet_list(ip_range))

    session = boto3.Session(profile_name=args.profile)
    waf = session.client('waf')
    change_tokens = apply_updates_to_waf(subnet_list, waf)

    # wait for every change to finish before ending
    waiting_on_changes = True
    while waiting_on_changes:
        change_in_progress = False
        time.sleep(.3)
        for token in change_tokens:
            status = waf.get_change_token_status(ChangeToken=token["ChangeToken"])
            change_in_progress = change_in_progress and (status == "PENDING")
            if change_in_progress:
                break
        waiting_on_changes = change_in_progress

    return not waiting_on_changes


def apply_updates_to_waf(ip_list, waf):
    white_list_rule = get_ip_whitelist_rule(waf)
    ip_set_list, negate_conditions = get_ip_sets_from_rule(white_list_rule, waf)

    ip_to_add = list(ip_list)
    adds = list()

    change_tokens = list()

    # for each ip set, get the ip ranges in each descriptor and compare against the ip list
    # remove duplicates from the list to add
    for ip_set in ip_set_list:
        for ip_range_descriptor in ip_set["IPSetDescriptors"]:
            ip = netaddr.IPNetwork(ip_range_descriptor["Value"])
            # if an ip in the list is already in an IPSet, then remove it from the list of stuff to add
            if ip in ip_list:
                ip_to_add.remove(ip)

    # create add operations
    for ip in ip_to_add:
        adds.append(
            {
                'Action': 'INSERT',
                'IPSetDescriptor': {
                    'Type': ip_version,
                    'Value': str(ip)
                }
            }
        )

    # make delete operations for anything in a set that isn't in the list of IPs, then populate the rest of the list
    #   with adds until we hit max of the IPSet
    for ip_set in ip_set_list:
        removes = list()

        for ip_range_descriptor in ip_set["IPSetDescriptors"]:
            ip = netaddr.IPNetwork(ip_range_descriptor["Value"])
            if ip not in ip_list:
                removes.append(
                    {
                        'Action': 'DELETE',
                        'IPSetDescriptor': ip_range_descriptor
                    }
                )

        # perform the updates to this set
        perform_updates_to_existing_ipset(ip_set, removes, adds, waf, change_tokens)

    # we've done all the removes, and filled all existing ip_sets with adds. if we have leftovers, we need to make a
    #   new ip_set and add it to the rule
    if len(adds) > 0:
        num_ip_sets = len(ip_set_list)
        rule_updates = list()
        while len(adds) > 0:
            create_token = waf.get_change_token()
            change_tokens.append(create_token)
            ip_set = waf.create_ip_set(Name="{0}_{1}".format(ip_set_basename, num_ip_sets),
                                       ChangeToken=create_token["ChangeToken"])["IPSet"]
            num_ip_sets += 1
            ip_set_id = ip_set["IPSetId"]
            rule_updates.append(
                {
                    'Action': 'INSERT',
                    'Predicate': {
                        'Negated': negate_conditions,
                        'Type': 'IPMatch',
                        'DataId': ip_set_id
                    }
                }
            )

            # populate the new ip_set
            batch = move_updates_to_batch(adds)
            update_set_token = waf.get_change_token()
            change_tokens.append(update_set_token)
            waf.update_ip_set(IPSetId=ip_set_id,
                              ChangeToken=update_set_token["ChangeToken"],
                              Updates=batch)

        # update the rule with the new ip_lists
        update_rule_token = waf.get_change_token()
        change_tokens.append(update_rule_token)
        waf.update_rule(RuleId=white_list_rule["RuleId"],
                        ChangeToken=update_rule_token["ChangeToken"],
                        Updates=rule_updates)

    return change_tokens


def perform_updates_to_existing_ipset(ip_set, removes, adds, waf, change_tokens):
    # figure out how many adds can be done on this ip_set after all of the removes
    space_remaining_in_set = max_ranges_per_ip_set - (len(ip_set["IPSetDescriptors"]) - len(removes))

    # fill the list of updates to perform with removes and adds until the end result is either a full set or
    #   all operations have been performed.
    updates = list(removes)
    updates.extend(list(adds[0:space_remaining_in_set]))
    adds[0:space_remaining_in_set] = []

    # make batches of updates (max per batch = max_ranges_per_update)
    update_batches = list()
    while len(updates) > 0:
        batch = move_updates_to_batch(updates)
        update_batches.append(batch)

    # submit an update set request for each batch
    for batch in update_batches:
        change_token = waf.get_change_token()
        change_tokens.append(change_token)
        waf.update_ip_set(IPSetId=ip_set["IPSetId"],
                          ChangeToken=change_token["ChangeToken"],
                          Updates=batch)


def move_updates_to_batch(original_list):
    items_to_batch = min(len(original_list), max_ranges_per_update)
    batch = list(original_list[0:items_to_batch])
    original_list[0:items_to_batch] = []
    return batch


def get_ip_whitelist_rule(waf_client):
    rules_list = waf_client.list_rules(Limit=100)

    whitelist_rule_id = None
    for rule in rules_list["Rules"]:
        if rule["Name"] == rule_name:
            whitelist_rule_id = rule["RuleId"]

    if whitelist_rule_id is None:
        return None

    return waf_client.get_rule(RuleId=whitelist_rule_id)["Rule"]


def get_ip_sets_from_rule(rule, waf_client):
    ip_sets = list()
    negate_conditions = False
    for condition in rule["Predicates"]:
        if condition["Type"] == 'IPMatch':
            ip_set_id = condition["DataId"]
            ip_set = waf_client.get_ip_set(IPSetId=ip_set_id)

            if ip_set is not None and ip_set_basename in ip_set["IPSet"]["Name"]:
                ip_sets.append(ip_set["IPSet"])
            else:
                print "No IPSet with ID {0}".format(ip_set_id)

            negate_conditions = condition["Negated"]

    return ip_sets, negate_conditions


def translate_range_to_subnet_list(ip_range):
    """

    :param ip_range: The IP address + CIDR prefix (IPNetwork object)
    :return: A list of all WAF-valid subnets that compose the given IP range
    """
    prefix = ip_range.prefixlen
    waf_valid_prefix = find_closest_waf_range(prefix)
    return list(ip_range.subnet(waf_valid_prefix))


def find_closest_waf_range(cidr_prefix_length):
    """
    AWS WAF only accepts CIDR ranges of /8, /16, /24, /32. figure out what the closest safe range we need to convert to.
    This will always return a smaller range than what is passed in for security reasons.

    :param cidr_prefix_length: The arbitrary CIDR range to convert to a WAF-valid range
    :return: The closest WAF-valid range. i.e. if cidr_prefix_length = 18, this fuction will return 24
    """
    multiple = math.trunc(cidr_prefix_length / 8)
    return (multiple + 1) * 8   # needs to be 1 based to get accurate range


def list_all_waf_valid_subnets(ip_list):
    num_ips = 0
    for ip_range in ip_list:
        subnets = translate_range_to_subnet_list(ip_range)
        num_ips += len(subnets)
        for ip in subnets:
            print ip
    return num_ips


def main():
    # IPs taken from https://w.amazon.com/index.php/PublicIPRanges
    ips = ["207.171.176.0/20",  # SEA
           "205.251.224.0/22",
           "176.32.120.0/22",
           "54.240.196.0/24",
           "54.231.244.0/22",
           "52.95.52.0/22",
           "205.251.232.0/22",  # PDX
           "54.240.230.0/23",
           "54.240.248.0/21",
           "54.231.160.0/19",
           "54.239.2.0/23",
           "54.239.48.0/22",
           "52.93.12.0/22",
           "52.94.208.0/21",
           "52.218.128.0/17",
           "204.246.160.0/22",  # SFO
           "205.251.228.0/22",
           "176.32.112.0/21",
           "54.240.198.0/24",
           "54.231.232.0/21",
           "52.219.20.0/22",
           "52.219.24.0/21"]

    args = parse_args()

    ip_objects = list(netaddr.IPNetwork(ip) for ip in ips)

    if args.list:
        print list_all_waf_valid_subnets(ip_objects)
    else:
        return translate_and_apply_to_waf(ip_objects, args)

if __name__ == '__main__':
    main()
