/* Generated by wayland-scanner 1.11.0 */

#ifndef SECURE_OUTPUT_UNSTABLE_V1_CLIENT_PROTOCOL_H
#define SECURE_OUTPUT_UNSTABLE_V1_CLIENT_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * @page page_secure_output_unstable_v1 The secure_output_unstable_v1 protocol
 * Protocol for providing secure output
 *
 * @section page_desc_secure_output_unstable_v1 Description
 *
 * This protocol specifies a set of interfaces used to prevent surface
 * contents from appearing in screenshots or from being visible on non-secure
 * outputs.
 *
 * In order to prevent surface contents from appearing in screenshots or from
 * being visible on non-secure outputs, a client must first bind the global
 * interface "wp_secure_output" which, if a compositor supports secure output,
 * is exposed by the registry. Using the bound global object, the client uses
 * the "get_security" request to instantiate an interface extension for a
 * wl_surface object. This extended interface will then allow surfaces
 * to be marked as only visible on secure outputs.
 *
 * Warning! The protocol described in this file is experimental and backward
 * incompatible changes may be made. Backward compatible changes may be added
 * together with the corresponding interface version bump. Backward
 * incompatible changes are done by bumping the version number in the protocol
 * and interface names and resetting the interface version. Once the protocol
 * is to be declared stable, the 'z' prefix and the version number in the
 * protocol and interface names are removed and the interface version number is
 * reset.
 *
 * @section page_ifaces_secure_output_unstable_v1 Interfaces
 * - @subpage page_iface_zcr_secure_output_v1 - secure output
 * - @subpage page_iface_zcr_security_v1 - security interface to a wl_surface
 * @section page_copyright_secure_output_unstable_v1 Copyright
 * <pre>
 *
 * Copyright 2016 The Chromium Authors.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * </pre>
 */
struct wl_surface;
struct zcr_secure_output_v1;
struct zcr_security_v1;

/**
 * @page page_iface_zcr_secure_output_v1 zcr_secure_output_v1
 * @section page_iface_zcr_secure_output_v1_desc Description
 *
 * The global interface exposing secure output capabilities is used
 * to instantiate an interface extension for a wl_surface object.
 * This extended interface will then allow surfaces to be marked as
 * as only visible on secure outputs.
 * @section page_iface_zcr_secure_output_v1_api API
 * See @ref iface_zcr_secure_output_v1.
 */
/**
 * @defgroup iface_zcr_secure_output_v1 The zcr_secure_output_v1 interface
 *
 * The global interface exposing secure output capabilities is used
 * to instantiate an interface extension for a wl_surface object.
 * This extended interface will then allow surfaces to be marked as
 * as only visible on secure outputs.
 */
extern const struct wl_interface zcr_secure_output_v1_interface;
/**
 * @page page_iface_zcr_security_v1 zcr_security_v1
 * @section page_iface_zcr_security_v1_desc Description
 *
 * An additional interface to a wl_surface object, which allows the
 * client to specify that a surface should not appear in screenshots
 * or be visible on non-secure outputs.
 *
 * If the wl_surface associated with the security object is destroyed,
 * the security object becomes inert.
 *
 * If the security object is destroyed, the security state is removed
 * from the wl_surface. The change will be applied on the next
 * wl_surface.commit.
 * @section page_iface_zcr_security_v1_api API
 * See @ref iface_zcr_security_v1.
 */
/**
 * @defgroup iface_zcr_security_v1 The zcr_security_v1 interface
 *
 * An additional interface to a wl_surface object, which allows the
 * client to specify that a surface should not appear in screenshots
 * or be visible on non-secure outputs.
 *
 * If the wl_surface associated with the security object is destroyed,
 * the security object becomes inert.
 *
 * If the security object is destroyed, the security state is removed
 * from the wl_surface. The change will be applied on the next
 * wl_surface.commit.
 */
extern const struct wl_interface zcr_security_v1_interface;

#ifndef ZCR_SECURE_OUTPUT_V1_ERROR_ENUM
#define ZCR_SECURE_OUTPUT_V1_ERROR_ENUM
enum zcr_secure_output_v1_error {
	/**
	 * the surface already has a security object associated
	 */
	ZCR_SECURE_OUTPUT_V1_ERROR_SECURITY_EXISTS = 0,
};
#endif /* ZCR_SECURE_OUTPUT_V1_ERROR_ENUM */

#define ZCR_SECURE_OUTPUT_V1_DESTROY	0
#define ZCR_SECURE_OUTPUT_V1_GET_SECURITY	1

/**
 * @ingroup iface_zcr_secure_output_v1
 */
#define ZCR_SECURE_OUTPUT_V1_DESTROY_SINCE_VERSION	1
/**
 * @ingroup iface_zcr_secure_output_v1
 */
#define ZCR_SECURE_OUTPUT_V1_GET_SECURITY_SINCE_VERSION	1

/** @ingroup iface_zcr_secure_output_v1 */
static inline void
zcr_secure_output_v1_set_user_data(struct zcr_secure_output_v1 *zcr_secure_output_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zcr_secure_output_v1, user_data);
}

/** @ingroup iface_zcr_secure_output_v1 */
static inline void *
zcr_secure_output_v1_get_user_data(struct zcr_secure_output_v1 *zcr_secure_output_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zcr_secure_output_v1);
}

static inline uint32_t
zcr_secure_output_v1_get_version(struct zcr_secure_output_v1 *zcr_secure_output_v1)
{
	return wl_proxy_get_version((struct wl_proxy *) zcr_secure_output_v1);
}

/**
 * @ingroup iface_zcr_secure_output_v1
 *
 * Informs the server that the client will not be using this
 * protocol object anymore. This does not affect any other objects,
 * security objects included.
 */
static inline void
zcr_secure_output_v1_destroy(struct zcr_secure_output_v1 *zcr_secure_output_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_secure_output_v1,
			 ZCR_SECURE_OUTPUT_V1_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zcr_secure_output_v1);
}

/**
 * @ingroup iface_zcr_secure_output_v1
 *
 * Instantiate an interface extension for the given wl_surface to
 * provide surface security. If the given wl_surface already has
 * a security object associated, the security_exists protocol error
 * is raised.
 */
static inline struct zcr_security_v1 *
zcr_secure_output_v1_get_security(struct zcr_secure_output_v1 *zcr_secure_output_v1, struct wl_surface *surface)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) zcr_secure_output_v1,
			 ZCR_SECURE_OUTPUT_V1_GET_SECURITY, &zcr_security_v1_interface, NULL, surface);

	return (struct zcr_security_v1 *) id;
}

#define ZCR_SECURITY_V1_DESTROY	0
#define ZCR_SECURITY_V1_ONLY_VISIBLE_ON_SECURE_OUTPUT	1

/**
 * @ingroup iface_zcr_security_v1
 */
#define ZCR_SECURITY_V1_DESTROY_SINCE_VERSION	1
/**
 * @ingroup iface_zcr_security_v1
 */
#define ZCR_SECURITY_V1_ONLY_VISIBLE_ON_SECURE_OUTPUT_SINCE_VERSION	1

/** @ingroup iface_zcr_security_v1 */
static inline void
zcr_security_v1_set_user_data(struct zcr_security_v1 *zcr_security_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zcr_security_v1, user_data);
}

/** @ingroup iface_zcr_security_v1 */
static inline void *
zcr_security_v1_get_user_data(struct zcr_security_v1 *zcr_security_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zcr_security_v1);
}

static inline uint32_t
zcr_security_v1_get_version(struct zcr_security_v1 *zcr_security_v1)
{
	return wl_proxy_get_version((struct wl_proxy *) zcr_security_v1);
}

/**
 * @ingroup iface_zcr_security_v1
 *
 * The associated wl_surface's security state is removed.
 * The change is applied on the next wl_surface.commit.
 */
static inline void
zcr_security_v1_destroy(struct zcr_security_v1 *zcr_security_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_security_v1,
			 ZCR_SECURITY_V1_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zcr_security_v1);
}

/**
 * @ingroup iface_zcr_security_v1
 *
 * Constrain visibility of wl_surface contents to secure outputs.
 * See wp_secure_output for the description.
 *
 * The only visible on secure output state is double-buffered state,
 * and will be applied on the next wl_surface.commit.
 */
static inline void
zcr_security_v1_only_visible_on_secure_output(struct zcr_security_v1 *zcr_security_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_security_v1,
			 ZCR_SECURITY_V1_ONLY_VISIBLE_ON_SECURE_OUTPUT);
}

#ifdef  __cplusplus
}
#endif

#endif
