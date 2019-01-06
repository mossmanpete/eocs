#include "eoc_relay_api_plugin.hpp"

#include <eosio/chain/exceptions.hpp>

#include <fc/io/json.hpp>

namespace eosio {

static appbase::abstract_plugin& _eoc_relay_api_plugin = app().register_plugin<eoc_relay_api_plugin>();
static bool _b_eoc_start = true;
eoc_relay_api_plugin::eoc_relay_api_plugin() {}
eoc_relay_api_plugin::~eoc_relay_api_plugin() {}

void eoc_relay_api_plugin::set_program_options(options_description&, options_description&cfg) {
   cfg.add_options()
      ("relay_plugin_type", bpo::value<int32_t>()->default_value(1), "The plugin start up type");
}

void eoc_relay_api_plugin::plugin_initialize(const variables_map& options) {
    int32_t startuptype = options.at("relay_plugin_type").as<int32_t>();
       if (startuptype == 1)
    { 
      _b_eoc_start= true;
    }
    else
    {
      _b_eoc_start= false;
    }
   
}

void eoc_relay_api_plugin::plugin_shutdown() {}

struct async_result_visitor : public fc::visitor<std::string> {
   template<typename T>
   std::string operator()(const T& v) const {
      return fc::json::to_string(v);
   }
};

#define CALL(api_name, api_handle, api_namespace, call_name, http_response_code) \
{std::string("/v1/" #api_name "/" #call_name), \
   [api_handle](string, string body, url_response_callback cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             auto result = api_handle.call_name(fc::json::from_string(body).as<api_namespace::call_name ## _params>()); \
             cb(http_response_code, fc::json::to_string(result)); \
          } catch (...) { \
             http_plugin::handle_exception(#api_name, #call_name, body, cb); \
          } \
       }}

#define CALL_ASYNC(api_name, api_handle, api_namespace, call_name, call_result, http_response_code) \
{std::string("/v1/" #api_name "/" #call_name), \
   [api_handle](string, string body, url_response_callback cb) mutable { \
      if (body.empty()) body = "{}"; \
      api_handle.call_name(fc::json::from_string(body).as<api_namespace::call_name ## _params>(),\
         [cb, body](const fc::static_variant<fc::exception_ptr, call_result>& result){\
            if (result.contains<fc::exception_ptr>()) {\
               try {\
                  result.get<fc::exception_ptr>()->dynamic_rethrow_exception();\
               } catch (...) {\
                  http_plugin::handle_exception(#api_name, #call_name, body, cb);\
               }\
            } else {\
               cb(http_response_code, result.visit(async_result_visitor()));\
            }\
         });\
   }\
}



#define EOC_RELAY_RO_CALL(call_name, http_response_code) CALL(eoc_icp, ro_api, eoc_icp::read_only, call_name, http_response_code)
#define EOC_RELAY_RW_CALL(call_name, http_response_code) CALL(eoc_icp, rw_api, eoc_icp::read_write, call_name, http_response_code)
#define EOC_RELAY_RW_CALL_ASYNC(call_name, call_result, http_response_code) CALL_ASYNC(eoc_icp, rw_api, eoc_icp::read_write, call_name, call_result, http_response_code)

void eoc_relay_api_plugin::plugin_startup() {
   ilog( "starting eoc_relay_api_plugin" );
   
   auto ro_api = app().get_plugin<eoc_relay_plugin>().get_read_only_api();
      auto rw_api = app().get_plugin<eoc_relay_plugin>().get_read_write_api();
      auto& _http_plugin = app().get_plugin<http_plugin>();
      _http_plugin.add_api({
         EOC_RELAY_RO_CALL(get_info, 200),
         EOC_RELAY_RO_CALL(get_block, 200),
         EOC_RELAY_RW_CALL(open_channel, 200)
      });
}

}