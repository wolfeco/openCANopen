/* Copyright (c) 2014-2016, Marel
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <errno.h>
#include "canopen/byteorder.h"
#include "canopen/sdo_sync.h"

struct sdo_req* sdo_sync_read(int nodeid, int index, int subindex)
{
	struct sdo_req_info info = {
		.type = SDO_REQ_UPLOAD,
		.index = index,
		.subindex = subindex
	};

	struct sdo_req* req = sdo_req_new(&info);
	if (!req)
		return NULL;

	if (sdo_req_start(req, sdo_req_queue_get(nodeid)) < 0)
		goto done;

	sdo_req_wait(req);

	if (req->status != SDO_REQ_OK) {
		errno = ENOENT;
		goto done;
	}

	return req;

done:
	sdo_req_unref(req);
	return NULL;
}

#define DECLARE_SDO_READ(type, name) \
type sdo_sync_read_ ## name(int nodeid, int index, int subindex) \
{ \
	type value = 0; \
	struct sdo_req* req = sdo_sync_read(nodeid, index, subindex); \
	if (!req) \
		return 0; \
	if (req->data.index > sizeof(value)) { \
		errno = ERANGE; \
		goto done; \
	} \
	byteorder2(&value, req->data.data, sizeof(value), req->data.index); \
done: \
	sdo_req_unref(req); \
	return value; \
}

DECLARE_SDO_READ(int64_t, i64)
DECLARE_SDO_READ(uint64_t, u64)
DECLARE_SDO_READ(int32_t, i32)
DECLARE_SDO_READ(uint32_t, u32)
DECLARE_SDO_READ(int16_t, i16)
DECLARE_SDO_READ(uint16_t, u16)
DECLARE_SDO_READ(int8_t, i8)
DECLARE_SDO_READ(uint8_t, u8)

int sdo_sync_write(int nodeid, struct sdo_req_info* info)
{
	int rc = -1;
	info->type = SDO_REQ_DOWNLOAD;

	struct sdo_req* req = sdo_req_new(info);
	if (!req)
		return -1;

	if (sdo_req_start(req, sdo_req_queue_get(nodeid)) < 0)
		goto failure;

	sdo_req_wait(req);

	if (req->status != SDO_REQ_OK)
		goto failure;

	rc = 0;
failure:
	sdo_req_unref(req);
	return rc;
}

#define DECLARE_SDO_WRITE(type, name) \
int sdo_sync_write_ ## name(int nodeid, struct sdo_req_info* info, type value) \
{ \
	type network_order = 0; \
	byteorder(&network_order, &value, sizeof(network_order)); \
	info->dl_data = &network_order; \
	info->dl_size = sizeof(network_order); \
	return sdo_sync_write(nodeid, info); \
}

DECLARE_SDO_WRITE(int64_t, i64)
DECLARE_SDO_WRITE(uint64_t, u64)
DECLARE_SDO_WRITE(int32_t, i32)
DECLARE_SDO_WRITE(uint32_t, u32)
DECLARE_SDO_WRITE(int16_t, i16)
DECLARE_SDO_WRITE(uint16_t, u16)
DECLARE_SDO_WRITE(int8_t, i8)
DECLARE_SDO_WRITE(uint8_t, u8)
