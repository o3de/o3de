CREATE DATABASE IF NOT EXISTS `CryAssetDB`;

USE `CryAssetDB`;

CREATE TABLE `categories` (
  `category_id` int(10) NOT NULL auto_increment,
  `category` varchar(255) UNIQUE NOT NULL,
  `order_id` int(10) NOT NULL,
  PRIMARY KEY  (`category_id`)
);

CREATE TABLE IF NOT EXISTS `tags` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `tag` varchar(64) NOT NULL,
  `category_id` int(10) default NULL,
  PRIMARY KEY (`id`),
  KEY `FK_tags_to_category_id` (`category_id`),
  UNIQUE KEY `tag` (`tag`),
  CONSTRAINT `FK_tags_to_category_id` FOREIGN KEY (`category_id`) REFERENCES `categories` (`category_id`) ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE TABLE IF NOT EXISTS `projects` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `name` varchar(32) NOT NULL COMMENT 'Convenient project name for easy filtering',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `name` (`name`)
);

CREATE TABLE IF NOT EXISTS `asset_inventory` (
  `id` bigint(20) unsigned NOT NULL auto_increment,
  `project_id` int(10) unsigned NOT NULL,
  `relpath` varchar(255) NOT NULL COMMENT 'relpath to the asset',
  `description` varchar(255) default NULL,
  PRIMARY KEY  (`id`),
  KEY `FK_asset_inventory_to_project_id` (`project_id`),
  CONSTRAINT `FK_asset_inventory_to_project_id` FOREIGN KEY (`project_id`) REFERENCES `projects` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE TABLE IF NOT EXISTS `asset_tags` (
  `asset_id` bigint(20) unsigned NOT NULL,
  `tag_id` int(10) unsigned NOT NULL,
  PRIMARY KEY (`asset_id`,`tag_id`),
  KEY `FK_asset_tags_to_asset_id` (`asset_id`),
  KEY `FK_asset_tags_to_tag_id` (`tag_id`),
  CONSTRAINT `FK_asset_tags_to_asset_id` FOREIGN KEY (`asset_id`) REFERENCES `asset_inventory` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `FK_asset_tags_to_tag_id` FOREIGN KEY (`tag_id`) REFERENCES `tags` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
)
