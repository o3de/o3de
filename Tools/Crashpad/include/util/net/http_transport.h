// Copyright 2014 The Crashpad Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CRASHPAD_UTIL_NET_HTTP_TRANSPORT_H_
#define CRASHPAD_UTIL_NET_HTTP_TRANSPORT_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "util/net/http_headers.h"

namespace crashpad {

class HTTPBodyStream;

//! \brief HTTPTransport executes a HTTP request using the specified URL, HTTP
//!     method, headers, and body. This class can only issue a synchronous
//!     HTTP request.
//!
//! This class cannot be instantiated directly. A concrete subclass must be
//! instantiated instead, which provides an implementation to execute the
//! request that is appropriate for the host operating system.
class HTTPTransport {
 public:
  virtual ~HTTPTransport();

  //! \brief Instantiates a concrete HTTPTransport class for the current
  //!     operating system.
  //!
  //! \return A new caller-owned HTTPTransport object.
  static std::unique_ptr<HTTPTransport> Create();

  //! \brief Sets URL to which the request will be made.
  //!
  //! \param[in] url The request URL.
  void SetURL(const std::string& url);

  //! \brief Sets the HTTP method to execute. E.g., GET, POST, etc. The default
  //!     method is `"POST"`.
  //!
  //! \param[in] http_method The HTTP method.
  void SetMethod(const std::string& http_method);

  //! \brief Sets a HTTP header-value pair.
  //!
  //! \param[in] header The HTTP header name. Any previous value set at this
  //!     name will be overwritten.
  //! \param[in] value The value to set for the header.
  void SetHeader(const std::string& header, const std::string& value);

  //! \brief Sets the stream object from which to generate the HTTP body.
  //!
  //! \param[in] stream A HTTPBodyStream, of which this class will take
  //!     ownership.
  void SetBodyStream(std::unique_ptr<HTTPBodyStream> stream);

  //! \brief Sets the timeout for the HTTP request. The default is 15 seconds.
  //!
  //! \param[in] timeout The request timeout, in seconds.
  void SetTimeout(double timeout);

  //! \brief Performs the HTTP request with the configured parameters and waits
  //!     for the execution to complete.
  //!
  //! \param[out] response_body On success, this will be set to the HTTP
  //!     response body. This parameter is optional and may be set to `nullptr`
  //!     if the response body is not required.
  //!
  //! \return Whether or not the request was successful, defined as returning
  //!     a HTTP status 200 (OK) code.
  virtual bool ExecuteSynchronously(std::string* response_body) = 0;

 protected:
  HTTPTransport();

  const std::string& url() const { return url_; }
  const std::string& method() const { return method_; }
  const HTTPHeaders& headers() const { return headers_; }
  HTTPBodyStream* body_stream() const { return body_stream_.get(); }
  double timeout() const { return timeout_; }

 private:
  std::string url_;
  std::string method_;
  HTTPHeaders headers_;
  std::unique_ptr<HTTPBodyStream> body_stream_;
  double timeout_;

  DISALLOW_COPY_AND_ASSIGN(HTTPTransport);
};

}  // namespace crashpad

#endif  // CRASHPAD_UTIL_NET_HTTP_TRANSPORT_H_
