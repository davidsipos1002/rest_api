/*
 * Copyright (c) 2023, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0, as
 * published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an
 * additional permission to link the program and your derivative works
 * with the separately licensed software that they have included with
 * MySQL.
 *
 * Without limiting anything contained in the foregoing, this file,
 * which is part of MySQL Connector/C++, is also subject to the
 * Universal FOSS Exception, version 1.0, a copy of which can be found at
 * http://oss.oracle.com/licenses/universal-foss-exception.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
 */

#include "mysql_telemetry.h"
#include "mysql_connection.h"
#include "mysql_statement.h"
#include "mysql_prepared_statement.h"

#include <cppconn/sqlstring.h>
#include <cppconn/version_info.h>

//#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace sql
{
namespace mysql
{
namespace telemetry
{


  Span_ptr mk_span(
    std::string name,
    std::optional<trace::SpanContext> link = {}
  )
  {
    auto tracer = trace::Provider::GetTracerProvider()->GetTracer(
      "MySQL Connector/C++", MYCPPCONN_DM_VERSION
    );

    trace::StartSpanOptions opts;
    opts.kind = trace::SpanKind::kClient;

    auto span
    = link ? tracer->StartSpan(name, {}, {{*link, {}}},  opts)
           : tracer->StartSpan(name, opts);

    span->SetAttribute("db.system", "mysql");
    return span;
  }

  std::string get_traceparent(Span_ptr &span)
  {
    char buf[trace::TraceId::kSize * 2];
    auto ctx = span->GetContext();

    ctx.trace_id().ToLowerBase16(buf);
    std::string trace_id{buf, sizeof(buf)};

    ctx.span_id().ToLowerBase16({buf, trace::SpanId::kSize * 2});
    std::string span_id{buf, trace::SpanId::kSize * 2};

    return "00-" + trace_id + "-" + span_id + "-00";
  }

  Span_ptr
  Telemetry_base<MySQL_Connection>::mk_span(MySQL_Connection*, const char*)
  {
    return telemetry::mk_span("connection");
  }


  /*
    Note: See [1] for relevant OTel semantic conventions for connection-level
    attributes.

    [1] https://github.com/open-telemetry/semantic-conventions/blob/main/docs/database/database-spans.md#connection-level-attributes
  */

  void
  Telemetry_base<MySQL_Connection>::set_attribs(
    MySQL_Connection* con,
    MySQL_Uri::Host_data& endpoint,
    sql::ConnectOptionsMap& options
  )
  {
    if (disabled(con) || !span)
      return;

    std::string transport;
    switch(endpoint.Protocol())
    {
      case NativeAPI::PROTOCOL_TCP:
        transport = "tcp";
        // TODO: If we can somehow detect IPv6 connections then "network.type"
        // should be set to "ipv6"
        span->SetAttribute("network.type", "ipv4");
        break;
      case NativeAPI::PROTOCOL_SOCKET:
        transport = "socket";
        span->SetAttribute("network.type", "unix");
      case NativeAPI::PROTOCOL_PIPE:
        transport = "pipe";
        break;
      default:
        transport = "other";
    }

    span->SetAttribute("network.transport", transport);
    span->SetAttribute("server.address", endpoint.Host().c_str());

    /*
      Note: `endpoint.hasPort()` alone is not good because it tells whether
      in a multi-host sceanrio a non-default port was specified for the chosen
      endpoint. We want to send the port attribute also when
      no endpint-specific port was given but user set the "global" port value
      which is used for all hosts (which is a more typical scenario).
    */

    if (options.count(OPT_PORT) || endpoint.hasPort())
    {
      span->SetAttribute("server.port", endpoint.Port());
    }
  }


  template<>
  bool
  Telemetry_base<MySQL_Statement>::disabled(MySQL_Statement *stmt) const
  {
    return stmt->conn_telemetry().disabled(stmt->connection);
  }

  /*
    Creating statement span: we link it to the connection span and we also
    set "traceparent" attribute unless user already set it.
  */

  template<>
  Span_ptr
  Telemetry_base<MySQL_Statement>::mk_span(MySQL_Statement *stmt,
    const char*)
  {
    auto span = telemetry::mk_span("SQL statement",
      stmt->conn_telemetry().span->GetContext()
    );

    /*
      Note: Parameter `false` means that an "internal" value for
      "traceparent" attribute is set that will not overwritte an existing
      "external" value of that attribute that was set by user.
    */

    setStmtAttrString(*stmt, "traceparent", get_traceparent(span), false);

    span->SetAttribute("db.user", stmt->connection->getCurrentUser().c_str());

    return span;
  }

  template<>
  bool
  Telemetry_base<MySQL_Prepared_Statement>::disabled(MySQL_Prepared_Statement *stmt) const
  {
    return stmt->conn_telemetry().disabled(stmt->connection);
  }

  template<>
  Span_ptr
  Telemetry_base<MySQL_Prepared_Statement>::mk_span(MySQL_Prepared_Statement *stmt,
    const char *name)
  {
    auto span = telemetry::mk_span( name == nullptr ? "SQL prepare" : name,
      stmt->conn_telemetry().span->GetContext()
    );

    if (name && !strncmp("SQL execute", name, 11))
    {
      // When setting STMT attribute for telemetry we need to signal that
      // it is not an external one. Otherwise it will be added as an attribute
      // set by user. This cannot be done without chaning the signature of
      // MySQL_Prepared_Statement::setQueryAttrString().
      // Therefore, we are calling a helper function, which knows how to add
      // such attributes.
      setStmtAttrString(*stmt, "traceparent", get_traceparent(span), false);
    }

    span->SetAttribute("db.user", stmt->connection->getCurrentUser().c_str());

    return span;
  }

} // telemetry
} // mysql
} // sql