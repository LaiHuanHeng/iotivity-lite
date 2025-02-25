/****************************************************************************
 *
 * Copyright (c) 2017-2019 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"),
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

#ifdef OC_SECURITY
#ifndef OC_DYNAMIC_ALLOCATION
#error "ERROR: Please rebuild with OC_DYNAMIC_ALLOCATION"
#endif /* !OC_DYNAMIC_ALLOCATION */

#include "api/oc_discovery_internal.h"
#include "api/oc_platform_internal.h"
#include "oc_core_res.h"
#include "oc_csr.h"
#include "oc_obt.h"
#include "oc_store.h"
#include "security/oc_acl_internal.h"
#include "security/oc_cred_internal.h"
#include "security/oc_cred_util_internal.h"
#include "security/oc_doxm_internal.h"
#include "security/oc_obt_internal.h"
#include "security/oc_pstat_internal.h"
#include "security/oc_sdi_internal.h"
#include "security/oc_sp_internal.h"
#include "security/oc_tls_internal.h"
#include "util/oc_macros_internal.h"

/* Just-works ownership transfer */
static void
obt_jw_16(oc_client_response_t *data)
{
  if (!oc_obt_is_otm_ctx_valid(data->user_data)) {
    return;
  }

  OC_DBG("In obt_jw_16");
  oc_otm_ctx_t *o = (oc_otm_ctx_t *)data->user_data;
  if (data->code >= OC_STATUS_BAD_REQUEST) {
    oc_obt_free_otm_ctx(o, -1, OC_OBT_OTM_JW);
    return;
  }

  /**  16) <close DTLS>
   */
  oc_obt_free_otm_ctx(o, 0, OC_OBT_OTM_JW);
}

static void
obt_jw_15(oc_client_response_t *data)
{
  if (!oc_obt_is_otm_ctx_valid(data->user_data)) {
    return;
  }

  OC_DBG("In obt_jw_15");
  oc_otm_ctx_t *o = (oc_otm_ctx_t *)data->user_data;
  if (data->code >= OC_STATUS_BAD_REQUEST) {
    goto err_obt_jw_15;
  }

  /**  15) post pstat s=rfnop
   */
  const oc_device_t *device = o->device;
  const oc_endpoint_t *ep = oc_obt_get_secure_endpoint(device->endpoint);
  if (oc_init_post("/oic/sec/pstat", ep, NULL, &obt_jw_16, HIGH_QOS, o)) {
    oc_rep_start_root_object();
    oc_rep_set_object(root, dos);
    oc_rep_set_int(dos, s, OC_DOS_RFNOP);
    oc_rep_close_object(root, dos);
    oc_rep_end_root_object();
    if (oc_do_post()) {
      return;
    }
  }

err_obt_jw_15:
  oc_obt_free_otm_ctx(o, -1, OC_OBT_OTM_JW);
}

static void
obt_jw_14(oc_client_response_t *data)
{
  if (!oc_obt_is_otm_ctx_valid(data->user_data)) {
    return;
  }

  OC_DBG("In obt_jw_14");
  oc_otm_ctx_t *o = (oc_otm_ctx_t *)data->user_data;
  if (data->code >= OC_STATUS_BAD_REQUEST) {
    goto err_obt_jw_14;
  }

  /**  14) post acl2 with ACEs for res, p, d, csr, sp
   */
  const oc_device_t *device = o->device;
  const oc_endpoint_t *ep = oc_obt_get_secure_endpoint(device->endpoint);
  if (oc_init_post("/oic/sec/acl2", ep, NULL, &obt_jw_15, HIGH_QOS, o)) {
    char uuid[OC_UUID_LEN];
    const oc_uuid_t *my_uuid = oc_core_get_device_id(0);
    oc_uuid_to_str(my_uuid, uuid, OC_UUID_LEN);

    oc_rep_start_root_object();

    oc_rep_set_array(root, aclist2);

    /* Owner-subejct ACEs for (R-only) /oic/sec/csr and (RW) /oic/sec/sp */
    oc_rep_object_array_start_item(aclist2);

    oc_rep_set_object(aclist2, subject);
    oc_rep_set_text_string(subject, uuid, uuid);
    oc_rep_close_object(aclist2, subject);

    oc_rep_set_array(aclist2, resources);

    oc_rep_object_array_start_item(resources);
    oc_rep_set_text_string(resources, href, OCF_SEC_SP_URI);
    oc_rep_object_array_end_item(resources);

    oc_rep_close_array(aclist2, resources);

    oc_rep_set_uint(aclist2, permission, 14);

    oc_rep_object_array_end_item(aclist2);
    /**/
    oc_rep_object_array_start_item(aclist2);

    oc_rep_set_object(aclist2, subject);
    oc_rep_set_text_string(subject, uuid, uuid);
    oc_rep_close_object(aclist2, subject);

    oc_rep_set_array(aclist2, resources);

    oc_rep_object_array_start_item(resources);
    oc_rep_set_text_string(resources, href, OCF_SEC_CSR_URI);
    oc_rep_object_array_end_item(resources);

    oc_rep_close_array(aclist2, resources);

    oc_rep_set_uint(aclist2, permission, 2);

    oc_rep_object_array_end_item(aclist2);

    /* anon-clear R-only ACE for res, d and p */
    oc_rep_object_array_start_item(aclist2);

    oc_rep_set_object(aclist2, subject);
    oc_rep_set_text_string(subject, conntype, "anon-clear");
    oc_rep_close_object(aclist2, subject);

    oc_rep_set_array(aclist2, resources);

    oc_rep_object_array_start_item(resources);
    oc_rep_set_text_string(resources, href, "/oic/d");
    oc_rep_object_array_end_item(resources);

    oc_rep_object_array_start_item(resources);
    oc_rep_set_text_string_v1(resources, href, OCF_PLATFORM_URI,
                              OC_CHAR_ARRAY_LEN(OCF_PLATFORM_URI));
    oc_rep_object_array_end_item(resources);

    oc_rep_object_array_start_item(resources);
    oc_rep_set_text_string_v1(resources, href, OCF_RES_URI,
                              OC_CHAR_ARRAY_LEN(OCF_RES_URI));
    oc_rep_object_array_end_item(resources);

    if (o->sdi) {
      oc_rep_object_array_start_item(resources);
      oc_rep_set_text_string_v1(resources, href, OCF_SEC_SDI_URI,
                                OC_CHAR_ARRAY_LEN(OCF_SEC_SDI_URI));
      oc_rep_object_array_end_item(resources);
    }

    oc_rep_close_array(aclist2, resources);

    oc_rep_set_uint(aclist2, permission, 0x02);

    oc_rep_object_array_end_item(aclist2);

    oc_rep_close_array(root, aclist2);

    oc_rep_end_root_object();
    if (oc_do_post()) {
      return;
    }
  }

err_obt_jw_14:
  oc_obt_free_otm_ctx(o, -1, OC_OBT_OTM_JW);
}

static void
obt_jw_13(oc_client_response_t *data)
{
  if (!oc_obt_is_otm_ctx_valid(data->user_data)) {
    return;
  }

  OC_DBG("In obt_jw_13");
  oc_otm_ctx_t *o = (oc_otm_ctx_t *)data->user_data;
  if (data->code >= OC_STATUS_BAD_REQUEST) {
    goto err_obt_jw_13;
  }

  /**  13) delete acl2
   */
  const oc_device_t *device = o->device;
  const oc_endpoint_t *ep = oc_obt_get_secure_endpoint(device->endpoint);
  if (oc_do_delete("/oic/sec/acl2", ep, NULL, &obt_jw_14, HIGH_QOS, o)) {
    return;
  }

err_obt_jw_13:
  oc_obt_free_otm_ctx(o, -1, OC_OBT_OTM_JW);
}

static void
obt_jw_12(oc_client_response_t *data)
{
  if (!oc_obt_is_otm_ctx_valid(data->user_data)) {
    return;
  }

  OC_DBG("In obt_jw_12");
  oc_otm_ctx_t *o = (oc_otm_ctx_t *)data->user_data;
  if (data->code >= OC_STATUS_BAD_REQUEST) {
    goto err_obt_jw_12;
  }

  /**  12) post pstat s=rfpro
   */
  const oc_device_t *device = o->device;
  const oc_endpoint_t *ep = oc_obt_get_secure_endpoint(device->endpoint);
  if (oc_init_post("/oic/sec/pstat", ep, NULL, &obt_jw_13, HIGH_QOS, o)) {
    oc_rep_start_root_object();
    oc_rep_set_object(root, dos);
    oc_rep_set_int(dos, s, OC_DOS_RFPRO);
    oc_rep_close_object(root, dos);
    oc_rep_end_root_object();
    if (oc_do_post()) {
      return;
    }
  }

err_obt_jw_12:
  oc_obt_free_otm_ctx(o, -1, OC_OBT_OTM_JW);
}

static void
obt_jw_11(oc_client_response_t *data)
{
  if (!oc_obt_is_otm_ctx_valid(data->user_data)) {
    return;
  }

  OC_DBG("In obt_jw_11");
  oc_otm_ctx_t *o = (oc_otm_ctx_t *)data->user_data;
  o->sdi = true;
  if (data->code >= OC_STATUS_BAD_REQUEST) {
    if (data->code != OC_STATUS_NOT_FOUND) {
      goto err_obt_jw_11;
    } else {
      o->sdi = false;
    }
  }

  oc_sec_dump_cred(0);

  /**  11) post doxm owned = true
   */
  const oc_device_t *device = o->device;
  const oc_endpoint_t *ep = oc_obt_get_secure_endpoint(device->endpoint);
  if (oc_init_post(OCF_SEC_DOXM_URI, ep, NULL, &obt_jw_12, HIGH_QOS, o)) {
    oc_rep_start_root_object();
    oc_rep_set_boolean(root, owned, true);
    oc_rep_end_root_object();
    if (oc_do_post()) {
      return;
    }
  }

err_obt_jw_11:
  oc_obt_free_otm_ctx(o, -1, OC_OBT_OTM_JW);
}

static void
obt_jw_10(oc_client_response_t *data)
{
  if (!oc_obt_is_otm_ctx_valid(data->user_data)) {
    return;
  }

  OC_DBG("In obt_jw_10");
  oc_otm_ctx_t *o = (oc_otm_ctx_t *)data->user_data;
  if (data->code >= OC_STATUS_BAD_REQUEST) {
    goto err_obt_jw_10;
  }

  const oc_sec_sdi_t *sdi = oc_sec_sdi_get(0);
  char sdi_uuid[OC_UUID_LEN];
  oc_uuid_to_str(&sdi->uuid, sdi_uuid, OC_UUID_LEN);

  /**  10) post sdi
   */
  const oc_device_t *device = o->device;
  const oc_endpoint_t *ep = oc_obt_get_secure_endpoint(device->endpoint);
  if (oc_init_post(OCF_SEC_SDI_URI, ep, NULL, &obt_jw_11, HIGH_QOS, o)) {
    oc_rep_start_root_object();
    oc_rep_set_text_string(root, uuid, sdi_uuid);
    oc_rep_set_text_string(root, name, oc_string(sdi->name));
    oc_rep_set_boolean(root, priv, sdi->priv);
    oc_rep_end_root_object();
    if (oc_do_post()) {
      return;
    }
  }

err_obt_jw_10:
  oc_obt_free_otm_ctx(o, -1, OC_OBT_OTM_JW);
}

static void
obt_jw_9(oc_client_response_t *data)
{
  if (!oc_obt_is_otm_ctx_valid(data->user_data)) {
    return;
  }

  OC_DBG("In obt_jw_9");
  oc_otm_ctx_t *o = (oc_otm_ctx_t *)data->user_data;
  if (data->code >= OC_STATUS_BAD_REQUEST) {
    goto err_obt_jw_9;
  }

  const oc_device_t *device = o->device;

  const oc_endpoint_t *ep = oc_obt_get_secure_endpoint(device->endpoint);
  const oc_uuid_t *my_uuid = oc_core_get_device_id(0);
  char uuid[OC_UUID_LEN];
  oc_uuid_to_str(my_uuid, uuid, OC_UUID_LEN);
  char suuid[OC_UUID_LEN];
  oc_uuid_to_str(&device->uuid, suuid, OC_UUID_LEN);

  uint8_t key[16];
  bool derived = oc_sec_derive_owner_psk(
    ep, (const uint8_t *)OC_OXMTYPE_JW_STR,
    OC_CHAR_ARRAY_LEN(OC_OXMTYPE_JW_STR), device->uuid.id,
    OC_ARRAY_SIZE(device->uuid.id), my_uuid->id, OC_ARRAY_SIZE(my_uuid->id),
    key, OC_ARRAY_SIZE(key));
  if (!derived) {
    goto err_obt_jw_9;
  }

  oc_sec_encoded_data_t privatedata = { key, OC_ARRAY_SIZE(key),
                                        OC_ENCODING_RAW };
  int credid =
    oc_sec_add_new_psk_cred(0, suuid, privatedata, OC_STRING_VIEW_NULL);
  if (credid == -1) {
    goto err_obt_jw_9;
  }

  oc_sec_cred_t *oc = oc_sec_get_cred_by_credid(credid, 0);
  if (oc) {
    oc->owner_cred = true;
  }

  /**  9) post cred rowneruuid, cred
   */
  if (oc_init_post("/oic/sec/cred", ep, NULL, &obt_jw_10, HIGH_QOS, o)) {
    oc_rep_start_root_object();
    oc_rep_set_array(root, creds);
    oc_rep_object_array_start_item(creds);

    oc_rep_set_int(creds, credtype, 1);
    oc_rep_set_text_string(creds, subjectuuid, uuid);

    oc_rep_set_object(creds, privatedata);
    oc_rep_set_text_string(privatedata, encoding, OC_ENCODING_RAW_STR);
    oc_rep_set_byte_string(privatedata, data, (const uint8_t *)"", 0);
    oc_rep_close_object(creds, privatedata);

    oc_rep_object_array_end_item(creds);
    oc_rep_close_array(root, creds);
    oc_rep_set_text_string(root, rowneruuid, uuid);
    oc_rep_end_root_object();
    if (oc_do_post()) {
      return;
    }
  }

err_obt_jw_9:
  oc_obt_free_otm_ctx(o, -1, OC_OBT_OTM_JW);
}

static void
obt_jw_8(oc_client_response_t *data)
{
  if (!oc_obt_is_otm_ctx_valid(data->user_data)) {
    return;
  }

  OC_DBG("In obt_jw_8");
  oc_otm_ctx_t *o = (oc_otm_ctx_t *)data->user_data;
  if (data->code >= OC_STATUS_BAD_REQUEST) {
    goto err_obt_jw_8;
  }

  /**  8) post pstat rowneruuid
   */
  const oc_device_t *device = o->device;
  const oc_endpoint_t *ep = oc_obt_get_secure_endpoint(device->endpoint);
  if (oc_init_post("/oic/sec/pstat", ep, NULL, &obt_jw_9, HIGH_QOS, o)) {
    const oc_uuid_t *my_uuid = oc_core_get_device_id(0);
    char uuid[OC_UUID_LEN];
    oc_uuid_to_str(my_uuid, uuid, OC_UUID_LEN);

    oc_rep_start_root_object();
    oc_rep_set_text_string(root, rowneruuid, uuid);
    oc_rep_end_root_object();
    if (oc_do_post()) {
      return;
    }
  }

err_obt_jw_8:
  oc_obt_free_otm_ctx(o, -1, OC_OBT_OTM_JW);
}

static void
obt_jw_7(oc_client_response_t *data)
{
  if (!oc_obt_is_otm_ctx_valid(data->user_data)) {
    return;
  }

  OC_DBG("In obt_jw_7");
  oc_otm_ctx_t *o = (oc_otm_ctx_t *)data->user_data;
  if (data->code >= OC_STATUS_BAD_REQUEST) {
    goto err_obt_jw_7;
  }

  /**  7) post acl rowneruuid
   */
  const oc_device_t *device = o->device;
  const oc_endpoint_t *ep = oc_obt_get_secure_endpoint(device->endpoint);

  if (oc_init_post("/oic/sec/acl2", ep, NULL, &obt_jw_8, HIGH_QOS, o)) {
    const oc_uuid_t *my_uuid = oc_core_get_device_id(0);
    char uuid[OC_UUID_LEN];
    oc_uuid_to_str(my_uuid, uuid, OC_UUID_LEN);

    oc_rep_start_root_object();
    oc_rep_set_text_string(root, rowneruuid, uuid);
    oc_rep_end_root_object();
    if (oc_do_post()) {
      return;
    }
  }

err_obt_jw_7:
  oc_obt_free_otm_ctx(o, -1, OC_OBT_OTM_JW);
}

static void
obt_jw_6(oc_client_response_t *data)
{
  if (!oc_obt_is_otm_ctx_valid(data->user_data)) {
    return;
  }

  OC_DBG("In obt_jw_6");
  oc_otm_ctx_t *o = (oc_otm_ctx_t *)data->user_data;
  if (data->code >= OC_STATUS_BAD_REQUEST) {
    goto err_obt_jw_6;
  }

  /**  6) post doxm rowneruuid
   */
  const oc_device_t *device = o->device;
  const oc_endpoint_t *ep = oc_obt_get_secure_endpoint(device->endpoint);

  if (oc_init_post(OCF_SEC_DOXM_URI, ep, NULL, &obt_jw_7, HIGH_QOS, o)) {
    const oc_uuid_t *my_uuid = oc_core_get_device_id(0);
    char uuid[OC_UUID_LEN];
    oc_uuid_to_str(my_uuid, uuid, OC_UUID_LEN);

    oc_rep_start_root_object();
    /* Set OBT's uuid as rowneruuid */
    oc_rep_set_text_string(root, rowneruuid, uuid);
    oc_rep_end_root_object();
    if (oc_do_post()) {
      return;
    }
  }

err_obt_jw_6:
  oc_obt_free_otm_ctx(o, -1, OC_OBT_OTM_JW);
}

static void
obt_jw_5(oc_client_response_t *data)
{
  if (!oc_obt_is_otm_ctx_valid(data->user_data)) {
    return;
  }

  OC_DBG("In obt_jw_5");
  oc_otm_ctx_t *o = (oc_otm_ctx_t *)data->user_data;
  if (data->code >= OC_STATUS_BAD_REQUEST) {
    goto err_obt_jw_5;
  }

  /** 5) generate random deviceuuid; <store new peer uuid>; post doxm deviceuuid
   */
  oc_uuid_t dev_uuid = { { 0 } };
  oc_gen_uuid(&dev_uuid);
  char uuid[OC_UUID_LEN];
  oc_uuid_to_str(&dev_uuid, uuid, OC_UUID_LEN);
  OC_DBG("generated deviceuuid: %s", uuid);

  oc_device_t *device = o->device;
  /* Store peer device's random uuid in local device object */
  memcpy(device->uuid.id, dev_uuid.id, OC_ARRAY_SIZE(dev_uuid.id));
  oc_endpoint_t *ep = device->endpoint;
  while (ep) {
    oc_endpoint_set_di(ep, &dev_uuid);
    ep = ep->next;
  }

  const oc_endpoint_t *cep = oc_obt_get_secure_endpoint(device->endpoint);
  if (oc_init_post(OCF_SEC_DOXM_URI, cep, NULL, &obt_jw_6, HIGH_QOS, o)) {
    oc_rep_start_root_object();
    /* Set random uuid as deviceuuid */
    oc_rep_set_text_string(root, deviceuuid, uuid);
    oc_rep_end_root_object();
    if (oc_do_post()) {
      return;
    }
  }

err_obt_jw_5:
  oc_obt_free_otm_ctx(o, -1, OC_OBT_OTM_JW);
}

static void
obt_jw_4(oc_client_response_t *data)
{
  if (!oc_obt_is_otm_ctx_valid(data->user_data)) {
    return;
  }

  OC_DBG("In obt_jw_4");
  oc_otm_ctx_t *o = (oc_otm_ctx_t *)data->user_data;
  if (data->code >= OC_STATUS_BAD_REQUEST) {
    goto err_obt_jw_4;
  }

  /** 4)  post doxm devowneruuid
   */
  const oc_device_t *device = o->device;
  const oc_endpoint_t *ep = oc_obt_get_secure_endpoint(device->endpoint);
  if (oc_init_post(OCF_SEC_DOXM_URI, ep, NULL, &obt_jw_5, HIGH_QOS, o)) {
    const oc_uuid_t *my_uuid = oc_core_get_device_id(0);
    char uuid[OC_UUID_LEN];
    oc_uuid_to_str(my_uuid, uuid, OC_UUID_LEN);

    oc_rep_start_root_object();
    /* Set OBT's uuid as devowneruuid */
    oc_rep_set_text_string(root, devowneruuid, uuid);
    oc_rep_end_root_object();
    if (oc_do_post()) {
      return;
    }
  }

err_obt_jw_4:
  oc_obt_free_otm_ctx(o, -1, OC_OBT_OTM_JW);
}

static void
obt_jw_3(oc_client_response_t *data)
{
  if (!oc_obt_is_otm_ctx_valid(data->user_data)) {
    return;
  }

  OC_DBG("In obt_jw_3");
  oc_otm_ctx_t *o = (oc_otm_ctx_t *)data->user_data;
  if (data->code >= OC_STATUS_BAD_REQUEST) {
    goto err_obt_jw_3;
  }

  /**  3) <Open-anon-ecdh>+post pstat om=4
   */
  const oc_device_t *device = o->device;
  const oc_endpoint_t *ep = oc_obt_get_secure_endpoint(device->endpoint);
  oc_tls_close_connection(ep);
  oc_tls_select_anon_ciphersuite();
  if (oc_init_post("/oic/sec/pstat", ep, NULL, &obt_jw_4, HIGH_QOS, o)) {
    oc_rep_start_root_object();
    oc_rep_set_int(root, om, 4);
    oc_rep_end_root_object();
    if (oc_do_post()) {
      return;
    }
  }

err_obt_jw_3:
  oc_obt_free_otm_ctx(o, -1, OC_OBT_OTM_JW);
}

static void
obt_jw_2(oc_client_response_t *data)
{
  if (!oc_obt_is_otm_ctx_valid(data->user_data)) {
    return;
  }

  OC_DBG("In obt_jw_2");
  oc_otm_ctx_t *o = (oc_otm_ctx_t *)data->user_data;
  if (data->code >= OC_STATUS_BAD_REQUEST) {
    goto err_obt_jw_2;
  }

  int64_t *oxms = NULL;
  size_t oxms_len = 0;

  if (oc_rep_get_int_array(data->payload, "oxms", &oxms, &oxms_len)) {
    size_t i;
    for (i = 0; i < oxms_len; i++) {
      if (oxms[i] == OC_OXMTYPE_JW) {
        break;
      }
    }

    if (i == oxms_len) {
      goto err_obt_jw_2;
    }

    /**  2) post doxm oxmsel=0
     */
    const oc_device_t *device = o->device;
    const oc_endpoint_t *ep = oc_obt_get_unsecure_endpoint(device->endpoint);
    if (oc_init_post(OCF_SEC_DOXM_URI, ep, NULL, &obt_jw_3, HIGH_QOS, o)) {
      oc_rep_start_root_object();
      oc_rep_set_int(root, oxmsel, 0);
      oc_rep_end_root_object();
      if (oc_do_post()) {
        return;
      }
    }
  }

err_obt_jw_2:
  oc_obt_free_otm_ctx(o, -1, OC_OBT_OTM_JW);
}

/*
  OTM sequence:
  1) get doxm
  2) post doxm oxmsel=0
  3) <Open-anon-ecdh>+post pstat om=4
  4) post doxm devowneruuid
  5) generate random deviceuuid; <store new peer uuid>; post doxm deviceuuid
  6) post doxm rowneruuid
  7) post acl rowneruuid
  8) post pstat rowneruuid
  9) post cred rowneruuid, cred
  10) post sdi
  11) post doxm owned = true
  12) post pstat s=rfpro
  13) delete acl2
  14) post acl2 with ACEs for res, p, d, csr, sp
  15) post pstat s=rfnop
  16) <close DTLS>
*/
int
oc_obt_perform_just_works_otm(const oc_uuid_t *uuid,
                              oc_obt_device_status_cb_t cb, void *data)
{
  OC_DBG("In oc_obt_perform_just_works_otm");

  oc_device_t *device = oc_obt_get_cached_device_handle(uuid);
  if (!device) {
    return -1;
  }

  if (oc_obt_is_owned_device(uuid)) {
    char subjectuuid[OC_UUID_LEN];
    oc_uuid_to_str(uuid, subjectuuid, OC_UUID_LEN);
    oc_cred_remove_by_subject(subjectuuid, 0);
  }

  oc_otm_ctx_t *o = oc_obt_alloc_otm_ctx();
  if (!o) {
    return -1;
  }

  o->cb.cb = cb;
  o->cb.data = data;
  o->device = device;

  /**  1) get doxm
   */
  const oc_endpoint_t *ep = oc_obt_get_unsecure_endpoint(device->endpoint);
  if (oc_do_get(OCF_SEC_DOXM_URI, ep, NULL, &obt_jw_2, HIGH_QOS, o)) {
    return 0;
  }

  oc_obt_free_otm_ctx(o, -1, OC_OBT_OTM_JW);

  return -1;
}

#endif /* OC_SECURITY */
