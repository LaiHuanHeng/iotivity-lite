/******************************************************************
 *
 * Copyright 2019 Jozef Kralik All Rights Reserved.
 * Copyright 2018 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"),
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************/

#include "api/cloud/oc_cloud_internal.h"
#include "api/cloud/oc_cloud_manager_internal.h"
#include "api/cloud/oc_cloud_store_internal.h"
#include "api/oc_rep_internal.h"
#include "messaging/coap/transactions_internal.h"
#include "oc_api.h"
#include "oc_rep.h"
#include "oc_cloud.h"
#include "port/oc_log_internal.h"
#include "tests/gtest/Device.h"
#include "tests/gtest/RepPool.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

#ifndef OC_SECURITY

class TestCloudManager : public testing::Test {
public:
  oc_cloud_context_t m_context;

  void SetUp() override
  {
    ASSERT_TRUE(oc::TestDevice::StartServer());

    memset(&m_context, 0, sizeof(m_context));
#define UID "501"
    oc_new_string(&m_context.store.uid, UID, strlen(UID));
    m_context.cloud_ep = oc_new_endpoint();
    memset(m_context.cloud_ep, 0, sizeof(oc_endpoint_t));
#define ENDPOINT "coap://224.0.1.187:5683"
    oc_new_string(&m_context.store.ci_server, ENDPOINT, strlen(ENDPOINT));
#define TOKEN "access_token"
    oc_new_string(&m_context.store.access_token, TOKEN, strlen(TOKEN));
#define RTOKEN "refresh_token"
    oc_new_string(&m_context.store.refresh_token, RTOKEN, strlen(RTOKEN));
  }

  void TearDown() override
  {
    oc_free_string(&m_context.store.refresh_token);
    oc_free_string(&m_context.store.access_token);
    oc_free_string(&m_context.store.ci_server);
    oc_free_endpoint(m_context.cloud_ep);
    oc_free_string(&m_context.store.uid);

    oc::TestDevice::Reset();
    oc::TestDevice::StopServer();
  }

  static void schedule_stop_cloud_manager(oc_cloud_context_t *ctx)
  {
    oc_set_delayed_callback(
      ctx,
      [](void *data) -> oc_event_callback_retval_t {
        auto *ctx = static_cast<oc_cloud_context_t *>(data);
        cloud_manager_stop(ctx);
        return OC_EVENT_DONE;
      },
      0);
  }
};

TEST_F(TestCloudManager, cloud_manager_start_initialized_schedule_turnoff)
{
  // When
  oc_cloud_set_schedule_action(
    &m_context,
    [](oc_cloud_action_t, uint8_t, uint64_t *, uint16_t *, void *) -> bool {
      return false;
    },
    nullptr);

  m_context.store.status = OC_CLOUD_INITIALIZED;
  m_context.store.cps = OC_CPS_READYTOREGISTER;
  cloud_manager_start(&m_context);
  oc::TestDevice::PoolEvents(5);
  cloud_manager_stop(&m_context);

  // Then
  EXPECT_EQ(0, m_context.retry_count);
  EXPECT_EQ(0, m_context.retry_refresh_token_count);
  EXPECT_EQ(CLOUD_ERROR_CONNECT, m_context.last_error);
  EXPECT_EQ(OC_CLOUD_INITIALIZED, m_context.store.status);

  oc_cloud_set_schedule_action(&m_context, nullptr, nullptr);
}

TEST_F(TestCloudManager,
       cloud_manager_start_initialized_schedule_without_retry_and_access_token)
{
  // When
  oc_cloud_set_schedule_action(
    &m_context,
    [](oc_cloud_action_t action, uint8_t retry, uint64_t *delay,
       uint16_t *timeout, void *data) -> bool {
      auto *ctx = static_cast<oc_cloud_context_t *>(data);
      if (action == OC_CLOUD_ACTION_REGISTER && retry == 0) {
        *delay = 0;
        *timeout = 1;
        return true;
      }
      // to avoid override m_context.last_error and m_context.store.status
      schedule_stop_cloud_manager(ctx);
      return false;
    },
    &m_context);

  m_context.store.status = OC_CLOUD_INITIALIZED;
  m_context.store.cps = OC_CPS_READYTOREGISTER;
  oc_free_string(&m_context.store.access_token);

  cloud_manager_start(&m_context);
  oc::TestDevice::PoolEvents(5);
  cloud_manager_stop(&m_context);

  // Then
  EXPECT_EQ(1, m_context.retry_count);
  EXPECT_EQ(0, m_context.retry_refresh_token_count);
  EXPECT_EQ(CLOUD_ERROR_CONNECT, m_context.last_error);
  EXPECT_EQ(OC_CLOUD_INITIALIZED, m_context.store.status);

  oc_cloud_set_schedule_action(&m_context, nullptr, nullptr);
}

TEST_F(TestCloudManager, cloud_manager_start_initialized_without_retry_f)
{
  // When
  oc_cloud_set_schedule_action(
    &m_context,
    [](oc_cloud_action_t, uint8_t retry_count, uint64_t *delay,
       uint16_t *timeout, void *data) -> bool {
      auto *ctx = static_cast<oc_cloud_context_t *>(data);
      if (retry_count == 0) {
        *delay = 0;
        *timeout = 1;
        return true;
      }
      // to avoid override m_context.last_error and m_context.store.status
      schedule_stop_cloud_manager(ctx);
      return false;
    },
    &m_context);

  m_context.store.status = OC_CLOUD_INITIALIZED;
  m_context.store.cps = OC_CPS_READYTOREGISTER;
  cloud_manager_start(&m_context);
  oc::TestDevice::PoolEvents(5);
  cloud_manager_stop(&m_context);

  // Then
  EXPECT_LT(0, m_context.retry_count);
  EXPECT_EQ(0, m_context.retry_refresh_token_count);
  EXPECT_EQ(CLOUD_ERROR_CONNECT, m_context.last_error);
  EXPECT_EQ(OC_CLOUD_INITIALIZED, m_context.store.status);

  oc_cloud_set_schedule_action(&m_context, nullptr, nullptr);
}

TEST_F(TestCloudManager, cloud_manager_start_initialized_f)
{
  // When
  m_context.store.status = OC_CLOUD_INITIALIZED;
  m_context.store.cps = OC_CPS_READYTOREGISTER;
  cloud_manager_start(&m_context);
  oc::TestDevice::PoolEvents(5);
  cloud_manager_stop(&m_context);

  // Then
  EXPECT_LT(0, m_context.retry_count);
  EXPECT_EQ(0, m_context.retry_refresh_token_count);
  EXPECT_EQ(CLOUD_OK, m_context.last_error);
  EXPECT_EQ(OC_CLOUD_INITIALIZED, m_context.store.status);
}

TEST_F(TestCloudManager, cloud_manager_start_registered_without_retry_and_uid_f)
{
  // When
  oc_cloud_set_schedule_action(
    &m_context,
    [](oc_cloud_action_t action, uint8_t retry, uint64_t *delay,
       uint16_t *timeout, void *data) -> bool {
      auto *ctx = static_cast<oc_cloud_context_t *>(data);
      if (action == OC_CLOUD_ACTION_LOGIN && retry == 0) {
        *delay = 0;
        *timeout = 1;
        return true;
      }
      // to avoid override m_context.last_error and m_context.store.status
      schedule_stop_cloud_manager(ctx);
      return false;
    },
    &m_context);
  m_context.store.status = OC_CLOUD_INITIALIZED | OC_CLOUD_REGISTERED;
  m_context.store.expires_in = -1;
  oc_free_string(&m_context.store.uid);
  cloud_manager_start(&m_context);
  oc::TestDevice::PoolEvents(5);
  cloud_manager_stop(&m_context);

  // Then
  EXPECT_LT(0, m_context.retry_count);
  EXPECT_EQ(0, m_context.retry_refresh_token_count);
  EXPECT_EQ(CLOUD_ERROR_CONNECT, m_context.last_error);
  EXPECT_EQ(OC_CLOUD_INITIALIZED | OC_CLOUD_REGISTERED | OC_CLOUD_FAILURE,
            m_context.store.status);

  oc_cloud_set_schedule_action(&m_context, nullptr, nullptr);
}

TEST_F(TestCloudManager, cloud_manager_start_registered_f)
{
  // When
  m_context.store.status = OC_CLOUD_INITIALIZED | OC_CLOUD_REGISTERED;
  m_context.store.expires_in = -1;
  cloud_manager_start(&m_context);
  oc::TestDevice::PoolEvents(5);
  cloud_manager_stop(&m_context);

  // Then
  EXPECT_LT(0, m_context.retry_count);
  EXPECT_EQ(0, m_context.retry_refresh_token_count);
  EXPECT_EQ(CLOUD_OK, m_context.last_error);
  EXPECT_EQ(OC_CLOUD_INITIALIZED | OC_CLOUD_REGISTERED, m_context.store.status);
}

TEST_F(TestCloudManager,
       cloud_manager_start_with_refresh_token_without_uid_and_retry_f)
{
  // When
  oc_cloud_set_schedule_action(
    &m_context,
    [](oc_cloud_action_t action, uint8_t retry, uint64_t *delay,
       uint16_t *timeout, void *data) -> bool {
      auto *ctx = static_cast<oc_cloud_context_t *>(data);
      if (action == OC_CLOUD_ACTION_REFRESH_TOKEN && retry == 0) {
        *delay = 0;
        *timeout = 1;
        return true;
      }
      // to avoid override m_context.last_error and m_context.store.status
      schedule_stop_cloud_manager(ctx);
      return false;
    },
    &m_context);
  m_context.store.status = OC_CLOUD_INITIALIZED | OC_CLOUD_REGISTERED;
  oc_free_string(&m_context.store.uid);
  cloud_manager_start(&m_context);
  oc::TestDevice::PoolEvents(7);
  cloud_manager_stop(&m_context);

  // Then
  EXPECT_EQ(0, m_context.retry_count);
  EXPECT_LT(0, m_context.retry_refresh_token_count);
  EXPECT_EQ(CLOUD_ERROR_REFRESH_ACCESS_TOKEN, m_context.last_error);
  EXPECT_EQ(OC_CLOUD_INITIALIZED | OC_CLOUD_REGISTERED | OC_CLOUD_FAILURE,
            m_context.store.status);

  oc_cloud_set_schedule_action(&m_context, nullptr, nullptr);
}

TEST_F(TestCloudManager, cloud_manager_start_with_refresh_token_f)
{
  // When
  m_context.store.status = OC_CLOUD_INITIALIZED | OC_CLOUD_REGISTERED;
  cloud_manager_start(&m_context);
  oc::TestDevice::PoolEvents(7);
  cloud_manager_stop(&m_context);

  // Then
  EXPECT_EQ(0, m_context.retry_count);
  EXPECT_LT(0, m_context.retry_refresh_token_count);
  EXPECT_EQ(CLOUD_OK, m_context.last_error);
  EXPECT_EQ(OC_CLOUD_INITIALIZED | OC_CLOUD_REGISTERED, m_context.store.status);
}

#endif /* !OC_SECURITY */

class TestCloudManagerData : public testing::Test {
public:
  void SetUp() override { memset(&m_context, 0, sizeof(m_context)); }

  void TearDown() override { cloud_store_deinitialize(&m_context.store); }

  void Clear() { pool_.Clear(); }

  oc::oc_rep_unique_ptr ParsePayload() { return pool_.ParsePayload(); }

  static void PrintJson(const oc_rep_t *rep)
  {
    (void)rep;
#if OC_DBG_IS_ENABLED
    size_t json_size = oc_rep_to_json(rep, nullptr, 0, true);
    std::vector<char> json{};
    json.resize(json_size + 1);
    oc_rep_to_json(rep, &json[0], json.capacity(), true);
    OC_PRINTF("%s", json.data());
#endif /* OC_DBG_IS_ENABLED */
  }

  oc::oc_rep_unique_ptr GetPayload(const std::string &access_token,
                                   const std::string &refresh_token,
                                   const std::string &uid, int64_t expires_in);

  oc_cloud_context_t *GetContext()
  {
    return &m_context;
  }

  bool IsEmptyContext() const
  {
    return oc_string(m_context.store.ci_server) == nullptr &&
           oc_string(m_context.store.access_token) == nullptr &&
           oc_string(m_context.store.refresh_token) == nullptr &&
           oc_string(m_context.store.uid) == nullptr &&
           m_context.store.expires_in == 0 && m_context.store.status == 0;
  }

private:
  oc_cloud_context_t m_context{};
  oc::RepPool pool_{};
};

oc::oc_rep_unique_ptr
TestCloudManagerData::GetPayload(const std::string &access_token,
                                 const std::string &refresh_token = {},
                                 const std::string &uid = {},
                                 int64_t expires_in = -1)
{
  oc_rep_begin_root_object();
  EXPECT_EQ(CborNoError, oc_rep_get_cbor_errno());
  if (!access_token.empty()) {
    oc_rep_set_text_string(root, accesstoken, access_token.c_str());
    EXPECT_EQ(CborNoError, oc_rep_get_cbor_errno());
  }
  if (!refresh_token.empty()) {
    oc_rep_set_text_string(root, refreshtoken, refresh_token.c_str());
    EXPECT_EQ(CborNoError, oc_rep_get_cbor_errno());
  }
  if (!uid.empty()) {
    oc_rep_set_text_string(root, uid, uid.c_str());
    EXPECT_EQ(CborNoError, oc_rep_get_cbor_errno());
  }
  if (expires_in >= 0) {
    oc_rep_set_int(root, expiresin, expires_in);
    EXPECT_EQ(CborNoError, oc_rep_get_cbor_errno());
  }
  oc_rep_end_root_object();
  EXPECT_EQ(CborNoError, oc_rep_get_cbor_errno());
  oc::oc_rep_unique_ptr rep = ParsePayload();
  PrintJson(rep.get());
  return rep;
}

TEST_F(TestCloudManagerData, cloud_manager_parse_register_data_invalid)
{
  // {
  //   accesstoken: "accesstoken",
  // }
  EXPECT_FALSE(cloud_manager_handle_register_response(
    GetContext(), GetPayload("accesstoken").get()));
  EXPECT_TRUE(IsEmptyContext());
  Clear();

  // {
  //   refreshtoken: "refreshtoken",
  // }
  EXPECT_FALSE(cloud_manager_handle_register_response(
    GetContext(), GetPayload("", "refreshtoken").get()));
  EXPECT_TRUE(IsEmptyContext());
  Clear();

  // {
  //   accesstoken: "accesstoken",
  //   refreshtoken: "refreshtoken",
  // }
  EXPECT_FALSE(cloud_manager_handle_register_response(
    GetContext(), GetPayload("accesstoken", "refreshtoken").get()));
  EXPECT_TRUE(IsEmptyContext());
  Clear();

  // {
  //   accesstoken: "accesstoken",
  //   refreshtoken: "refreshtoken",
  //   uid: "uid",
  // }
  EXPECT_FALSE(cloud_manager_handle_register_response(
    GetContext(), GetPayload("accesstoken", "refreshtoken", "uid").get()));
  EXPECT_TRUE(IsEmptyContext());
  Clear();
}

TEST_F(TestCloudManagerData, cloud_manager_parse_register_data)
{
  // {
  //   accesstoken: "accesstoken",
  //   refreshtoken: "refreshtoken",
  //   uid: "uid",
  //   expiresin: 42,
  // }
  std::string at{ "accesstoken" };
  std::string rt{ "refreshtoken" };
  std::string uid{ "uid" };
  int64_t expiresin = 42;
  oc::oc_rep_unique_ptr rep = GetPayload(at, rt, uid, expiresin);
  EXPECT_TRUE(cloud_manager_handle_register_response(GetContext(), rep.get()));
  EXPECT_FALSE(IsEmptyContext());

  EXPECT_STREQ(at.c_str(), oc_string(GetContext()->store.access_token));
  EXPECT_STREQ(rt.c_str(), oc_string(GetContext()->store.refresh_token));
  EXPECT_STREQ(uid.c_str(), oc_string(GetContext()->store.uid));
  EXPECT_EQ(expiresin, GetContext()->store.expires_in);
}

TEST_F(TestCloudManagerData, cloud_manager_parse_redirect)
{
  std::string redirect{ "coap://mock.plgd.dev" };
  oc_rep_begin_root_object();
  EXPECT_EQ(CborNoError, oc_rep_get_cbor_errno());
  oc_rep_set_text_string(root, redirecturi, redirect.c_str());
  EXPECT_EQ(CborNoError, oc_rep_get_cbor_errno());
  oc_rep_end_root_object();
  EXPECT_EQ(CborNoError, oc_rep_get_cbor_errno());
  oc::oc_rep_unique_ptr rep = ParsePayload();
  PrintJson(rep.get());

  EXPECT_TRUE(cloud_manager_handle_redirect_response(GetContext(), rep.get()));
  EXPECT_FALSE(IsEmptyContext());
  EXPECT_STREQ(redirect.c_str(), oc_string(GetContext()->store.ci_server));
}

TEST_F(TestCloudManagerData, cloud_manager_parse_refresh_token_data_invalid)
{
  // {
  //   accesstoken: "accesstoken",
  // }
  EXPECT_FALSE(cloud_manager_handle_refresh_token_response(
    GetContext(), GetPayload("accesstoken").get()));
  EXPECT_TRUE(IsEmptyContext());
  Clear();

  // {
  //   refreshtoken: "refreshtoken",
  // }
  EXPECT_FALSE(cloud_manager_handle_refresh_token_response(
    GetContext(), GetPayload("", "refreshtoken").get()));
  EXPECT_TRUE(IsEmptyContext());
  Clear();

  // {
  //   accesstoken: "accesstoken",
  //   refreshtoken: "refreshtoken",
  // }
  EXPECT_FALSE(cloud_manager_handle_refresh_token_response(
    GetContext(), GetPayload("accesstoken", "refreshtoken").get()));
  EXPECT_TRUE(IsEmptyContext());
  Clear();
}

TEST_F(TestCloudManagerData, cloud_manager_parse_refresh_token_data)
{
  // {
  //   accesstoken: "accesstoken",
  //   refreshtoken: "refreshtoken",
  //   expiresin: 42,
  // }
  std::string at{ "accesstoken" };
  std::string rt{ "refreshtoken" };
  int64_t expiresin = 42;
  oc::oc_rep_unique_ptr rep = GetPayload(at, rt, "", expiresin);
  EXPECT_TRUE(
    cloud_manager_handle_refresh_token_response(GetContext(), rep.get()));
  EXPECT_FALSE(IsEmptyContext());

  EXPECT_STREQ(at.c_str(), oc_string(GetContext()->store.access_token));
  EXPECT_STREQ(rt.c_str(), oc_string(GetContext()->store.refresh_token));
  EXPECT_EQ(expiresin, GetContext()->store.expires_in);
}
