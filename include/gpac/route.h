/*
 *			GPAC - Multimedia Framework C SDK
 *
 *			Authors: Jean Le Feuvre
 *			Copyright (c) Telecom ParisTech 2017-2024
 *					All rights reserved
 *
 *  This file is part of GPAC / ROUTE (ATSC3, DVB-MABR) and DVB-MABR demuxer
 *
 *  GPAC is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  GPAC is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef _GF_ROUTE_H_
#define _GF_ROUTE_H_

#include <gpac/tools.h>

#ifndef GPAC_DISABLE_ROUTE

#ifdef __cplusplus
extern "C" {
#endif

/*!
\file <gpac/route.h>
\brief Specific extensions for ROUTE (ATSC3, DVB-I) protocol
*/

/*!
\addtogroup route_grp ROUTE
\ingroup media_grp
\brief ROUTE ATSC 3.0 receiver

The ROUTE receiver implements part of the ATSC 3.0 specification, mostly low-level signaling and ROUTE reception.
It gathers objects from a ROUTE session and sends them back to the user through a callback, or deletes them if no callback is sent.
The route demuxer does not try to repairing files, it is the user responsibility to do so.

@{
*/


/*! ATSC3.0 bootstrap address for LLS*/
#define GF_ATSC_MCAST_ADDR	"224.0.23.60"
/*! ATSC3.0 bootstrap port  for LLS*/
#define GF_ATSC_MCAST_PORT	4937

/*!The GF_ROUTEDmx object.*/
typedef struct __gf_routedmx GF_ROUTEDmx;

/*!The types of events used to communicate withe the demuxer user.*/
typedef enum
{
	/*! A new service is detected, service ID is in evt_param, no file info*/
	GF_ROUTE_EVT_SERVICE_FOUND = 0,
	/*! Service scan completed, no evt_param, no file info*/
	GF_ROUTE_EVT_SERVICE_SCAN,
	/*! New MPD or HLS master playlist available for service, service ID is in evt_param, file info carries manifest info*/
	GF_ROUTE_EVT_MPD,
	/*! HLS variant update for service, service ID is in evt_param, file info carries variant info*/
	GF_ROUTE_EVT_HLS_VARIANT,
	/*! static file update (with predefined TOI), service ID is in evt_param*/
	GF_ROUTE_EVT_FILE,
	/*! Segment reception, identified through a file template, service ID is in evt_param*/
	GF_ROUTE_EVT_DYN_SEG,
    /*! fragment reception (part of a segment), identified through a file template, service ID is in evt_param
     \note The data always begins at the start of the object
    */
    GF_ROUTE_EVT_DYN_SEG_FRAG,
    /*! Object deletion (only for dynamic TOIs), used to notify the cache that an object is no longer available. File info only contains the filename being removed*/
    GF_ROUTE_EVT_FILE_DELETE,
	/*! Delayed data reception */
	GF_ROUTE_EVT_LATE_DATA,
} GF_ROUTEEventType;

enum
{
	/*No-operation extension header*/
	GF_LCT_EXT_NOP = 0,
	/*Authentication extension header*/
	GF_LCT_EXT_AUTH = 1,
	/*Time extension header*/
	GF_LCT_EXT_TIME = 2,
	/*FEC object transmission information extension header*/
	GF_LCT_EXT_FTI = 64,
	/*Extension header for FDT - FLUTE*/
	GF_LCT_EXT_FDT = 192,
	/*Extension header for FDT content encoding - FLUTE*/
	GF_LCT_EXT_CENC = 193,
	/*TOL extension header - ROUTE - 24 bit payload*/
	GF_LCT_EXT_TOL24 = 194,
	/*TOL extension header - ROUTE - HEL + 28 bit payload*/
	GF_LCT_EXT_TOL48 = 67,
};

/*! LCT fragment information*/
typedef struct
{
    /*! offset in bytes of fragment in object / file*/
    u32 offset;
    /*! size in bytes of fragment in object / file*/
    u32 size;
} GF_LCTFragInfo;


/*! Type of partial event*/
typedef enum
{
	/* object is done receiving*/
	GF_LCTO_PARTIAL_NONE=0,
	/* object data being notified is the beginning of the payload*/
	GF_LCTO_PARTIAL_BEGIN,
	/* object data being notified is the complete reception buffer (for low latency mode), POTENTIALLY with holes in it*/
	GF_LCTO_PARTIAL_ANY,
} GF_LCTObjectPartial;

/*! Structure used to communicate file objects properties to the user*/
typedef struct
{
	/*! original file name*/
	const char *filename;
	/*! mime type if known, NULL otherwise*/
	const char *mime;
	/*! blob data pointer - the route user is responsible for setting the blob flags if desired*/
	GF_Blob *blob;
    /*! total size of object if known, 0 otherwise (TOL not received for route, last fragment not received for mabr+flute)*/
    u32 total_size;
	/*! object TSI*/
	u32 tsi;
	/*! object TOI*/
	u32 toi;
	/*! start time in ms*/
	u32 start_time;
	/*! download time in ms*/
	u32 download_ms;
	/*! flag set if file content has been modified - not set for GF_ROUTE_EVT_DYN_SEG (always true)*/
	Bool updated;
	/*! flag set if first segment has been received for the given TSI - not set for init segments*/
	Bool first_toi_received;

	/*! number of fragments, only set for GF_ROUTE_EVT_DYN_SEG*/
	u32 nb_frags;
	/*! fragment info, set for all file events - this info is shared with the LCT object being reassembled and should not be modified concurrently from route demux
	Any reallocation of the fragment info SHALL be done using  \ref gf_route_dmx_patch_frag_info
	*/
	GF_LCTFragInfo *frags;

	/*! offset of late received data, only for GF_ROUTE_EVT_LATE_DATA*/
	u32 late_fragment_offset;

	/*! for DASH,period ID, NULL otherwise*/
	char *dash_period_id;
	/*! for DASH, AS ID, -1 otherwise*/
	s32 dash_as_id;
	/*! for DASH, Representation ID, for HLS variant name, NULL otherwise*/
	char *dash_rep_id;

	/*partial state used for all calls
		if event indicates a file transfer completion (GF_ROUTE_EVT_FILE, GF_ROUTE_EVT_DYN_SEG), this relects the corrupted state
		of the reception
	*/
	GF_LCTObjectPartial partial;

	/*! user data set to current object after callback, and passed back on next callbacks on same object
	 Only used for GF_ROUTE_EVT_FILE, GF_ROUTE_EVT_DYN_SEG, GF_ROUTE_EVT_DYN_SEG_FRAG and GF_ROUTE_EVT_FILE_DELETE
	 */
	void *udta;
} GF_ROUTEEventFileInfo;

/*! Creates a new ROUTE ATSC3.0 demultiplexer
\param ifce network interface to monitor, NULL for INADDR_ANY
\param sock_buffer_size default buffer size for the udp sockets. If 0, uses 0x2000
\param on_event the user callback function
\param udta the user data passed back by the callback
\return the ROUTE demultiplexer created
*/
GF_ROUTEDmx *gf_route_atsc_dmx_new(const char *ifce, u32 sock_buffer_size, void (*on_event)(void *udta, GF_ROUTEEventType evt, u32 evt_param, GF_ROUTEEventFileInfo *finfo), void *udta);


/*! Creates a new ROUTE ATSC3.0 demultiplexer
\param ifce network interface to monitor, NULL for INADDR_ANY
\param sock_buffer_size default buffer size for the udp sockets. If 0, uses 0x2000
\param netcap_id ID of netcap configuration to use, may be null (see gpac -h netcap)
\param on_event the user callback function
\param udta the user data passed back by the callback
\return the ROUTE demultiplexer created
*/
GF_ROUTEDmx *gf_route_atsc_dmx_new_ex(const char *ifce, u32 sock_buffer_size, const char *netcap_id, void (*on_event)(void *udta, GF_ROUTEEventType evt, u32 evt_param, GF_ROUTEEventFileInfo *finfo), void *udta);


/*! Creates a new ROUTE demultiplexer
\param ip IP address of ROUTE session
\param port port of ROUTE session
\param ifce network interface to monitor, NULL for INADDR_ANY
\param sock_buffer_size default buffer size for the udp sockets. If 0, uses 0x2000
\param on_event the user callback function
\param udta the user data passed back by the callback
\return the ROUTE demultiplexer created
*/
GF_ROUTEDmx *gf_route_dmx_new(const char *ip, u32 port, const char *ifce, u32 sock_buffer_size, void (*on_event)(void *udta, GF_ROUTEEventType evt, u32 evt_param, GF_ROUTEEventFileInfo *finfo), void *udta);


/*! Creates a new ROUTE demultiplexer
\param ip IP address of ROUTE session
\param port port of ROUTE session
\param ifce network interface to monitor, NULL for INADDR_ANY
\param sock_buffer_size default buffer size for the udp sockets. If 0, uses 0x2000
\param netcap_id ID of netcap configuration to use, may be null (see gpac -h netcap)
\param on_event the user callback function
\param udta the user data passed back by the callback
\return the ROUTE demultiplexer created
*/
GF_ROUTEDmx *gf_route_dmx_new_ex(const char *ip, u32 port, const char *ifce, u32 sock_buffer_size, const char *netcap_id, void (*on_event)(void *udta, GF_ROUTEEventType evt, u32 evt_param, GF_ROUTEEventFileInfo *finfo), void *udta);

/*! Creates a new DVB MABR Flute demultiplexer
\param ip IP address of LCT session carrying the initial FDT
\param port port of LCT session carrying the initial FDT
\param ifce network interface to monitor, NULL for INADDR_ANY
\param sock_buffer_size default buffer size for the udp sockets. If 0, uses 0x2000
\param netcap_id ID of netcap configuration to use, may be null (see gpac -h netcap)
\param on_event the user callback function
\param udta the user data passed back by the callback
\return the demultiplexer created
*/
GF_ROUTEDmx *gf_dvb_mabr_dmx_new(const char *ip, u32 port, const char *ifce, u32 sock_buffer_size, const char *netcap_id, void (*on_event)(void *udta, GF_ROUTEEventType evt, u32 evt_param, GF_ROUTEEventFileInfo *finfo), void *udta);

/*! Deletes an ROUTE demultiplexer
\param routedmx the ROUTE demultiplexer to delete
*/
void gf_route_dmx_del(GF_ROUTEDmx *routedmx);

/*! Processes demultiplexing, returns when nothing to read
\param routedmx the ROUTE demultiplexer
\return error code if any, GF_IP_NETWORK_EMPTY if nothing was read
 */
GF_Err gf_route_dmx_process(GF_ROUTEDmx *routedmx);


/*! Checks if there are some active multicast sockets
\param routedmx the ROUTE demultiplexer
\return GF_TRUE if some multicast sockets are active, GF_FALSE otherwise
 */
Bool gf_route_dmx_has_active_multicast(GF_ROUTEDmx *routedmx);

/*! Checks for object being timeouts - this should only be called when \ref gf_route_dmx_process returns GF_IP_NETWORK_EMPTY for the first time in a batch
\param routedmx the ROUTE demultiplexer
\return GF_TRUE if some multicast sockets are active, GF_FALSE otherwise
 */
void gf_route_dmx_check_timeouts(GF_ROUTEDmx *routedmx);

/*! Sets reordering on.
\param routedmx the ROUTE demultiplexer
\param reorder_needed if TRUE, the order flag in ROUTE/LCT is ignored and objects are gathered for the given time. Otherwise, if order flag is set in ROUTE/LCT, an object is considered done as soon as a new object starts
\param timeout_us maximum delay in microseconds to wait before considering the object is done when ROUTE/LCT order is not used. A value of 0 implies any out-of-order packet triggers a download completion  (default value is 1 ms).
\return error code if any
 */
GF_Err gf_route_dmx_set_reorder(GF_ROUTEDmx *routedmx, Bool reorder_needed, u32 timeout_us);

/*! Progressive dispatch mode for LCT objects*/
typedef enum
{
	/*! notification is only sent once the entire object is received*/
	GF_ROUTE_DISPATCH_FULL = 0,
	/*! notifications are sent whenever the first byte-range starting at 0 changes, in which case the partial field is set to GF_LCTO_PARTIAL_BEGIN*/
	GF_ROUTE_DISPATCH_PROGRESSIVE,
	/*! notifications are sent whenever a new packet is received, in which case the partial field is set to GF_LCTO_PARTIAL_ANY*/
	GF_ROUTE_DISPATCH_OUT_OF_ORDER,
} GF_RouteProgressiveDispatch;

/*! Allow segments to be sent while being downloaded.

\note Files with a static TOI association are always sent once completely received, other files using TOI templating may be sent while being received if enabled. The data sent is always contiguous data since the beginning of the file in that case.

\param routedmx the ROUTE demultiplexer
\param dispatch_mode set dispatch mode
\return error code if any
 */
GF_Err gf_route_set_dispatch_mode(GF_ROUTEDmx *routedmx, GF_RouteProgressiveDispatch dispatch_mode);

/*! Sets the service ID to tune into for ATSC 3.0
\param routedmx the ROUTE demultiplexer
\param service_id ID of the service to tune in. 0 means no service, 0xFFFFFFFF means all services and 0xFFFFFFFE means first service found
\param tune_others if set, will tune all non-selected services to get the MPD, but won't receive any media data
\return error code if any
 */
GF_Err gf_route_atsc3_tune_in(GF_ROUTEDmx *routedmx, u32 service_id, Bool tune_others);


/*! Gets the number of objects currently loaded in the service
\param routedmx the ROUTE demultiplexer
\param service_id ID of the service to query
\return number of objects in service
 */
u32 gf_route_dmx_get_object_count(GF_ROUTEDmx *routedmx, u32 service_id);

/*! Removes an object with a given filename
\param routedmx the ROUTE demultiplexer
\param service_id ID of the service to query
\param fileName name of the file associated with the object
\param purge_previous if set, indicates that all objects with the same TSI and a TOI less than TOI of the deleted object will be removed
\return error if any, GF_NOT_FOUND if no such object
 */
GF_Err gf_route_dmx_remove_object_by_name(GF_ROUTEDmx *routedmx, u32 service_id, char *fileName, Bool purge_previous);

/*! Flags an object to be kept until \ref gf_route_dmx_remove_object_by_name is called
\param routedmx the ROUTE demultiplexer
\param service_id ID of the service to query
\param fileName name of the file associated with the object
\return error if any, GF_NOT_FOUND if no such object
 */
GF_Err gf_route_dmx_force_keep_object_by_name(GF_ROUTEDmx *routedmx, u32 service_id, char *fileName);

/*! Set force-keep flag on object by TSI and TOI - typically used for repair
\param routedmx the ROUTE demultiplexer
\param service_id ID of the service to query
\param tsi transport service identifier
\param toi transport object identifier
\param force_keep force_keep flag. When set back to false, this does not trigger a cleanup, it is up to the application to do so
\return error if any, GF_NOT_FOUND if no such object
 */
GF_Err gf_route_dmx_force_keep_object(GF_ROUTEDmx *routedmx, u32 service_id, u32 tsi, u32 toi, Bool force_keep);

/*! Removes the first object loaded in the service
\param routedmx the ROUTE demultiplexer
\param service_id ID of the service to query
\return GF_TRUE if success, GF_FALSE if no object could be removed (the object is in download)
 */
Bool gf_route_dmx_remove_first_object(GF_ROUTEDmx *routedmx, u32 service_id);

/*! Checks existence of a service for atsc 3.0
\param routedmx the ROUTE demultiplexer
\param service_id ID of the service to query
\return true if service is found, false otherwise
 */
Bool gf_route_dmx_find_atsc3_service(GF_ROUTEDmx *routedmx, u32 service_id);

/*! Removes all non-signaling objects (ie TSI!=0), keeping only init segments and currently/last downloaded objects
\note this is mostly useful in case of looping session, or at MPD switch boundaries
\param routedmx the ROUTE demultiplexer
\param service_id ID of the service to cleanup
 */
void gf_route_dmx_purge_objects(GF_ROUTEDmx *routedmx, u32 service_id);


/*! Gets high resolution system time clock of the first packet received
\param routedmx the ROUTE demultiplexer
\return system clock in microseconds of first packet received
 */
u64 gf_route_dmx_get_first_packet_time(GF_ROUTEDmx *routedmx);

/*! Gets high resolution system time clock of the last packet received
\param routedmx the ROUTE demultiplexer
\return system clock in microseconds of last packet received
 */
u64 gf_route_dmx_get_last_packet_time(GF_ROUTEDmx *routedmx);

/*! Gets the number of packets received since start of the session, for all active services
\param routedmx the ROUTE demultiplexer
\return number of packets received
 */
u64 gf_route_dmx_get_nb_packets(GF_ROUTEDmx *routedmx);

/*! Gets the number of bytes received since start of the session, for all active services
\param routedmx the ROUTE demultiplexer
\return number of bytes received
 */
u64 gf_route_dmx_get_recv_bytes(GF_ROUTEDmx *routedmx);

/*! Gather only objects with given TSI (for debug purposes)
\param routedmx the ROUTE demultiplexer
\param tsi the target TSI, 0 for no filtering
 */
void gf_route_dmx_debug_tsi(GF_ROUTEDmx *routedmx, u32 tsi);

/*! Sets udta for given service id
\param routedmx the ROUTE demultiplexer
\param service_id the target service
\param udta the target user data
 */
void gf_route_dmx_set_service_udta(GF_ROUTEDmx *routedmx, u32 service_id, void *udta);

/*! Gets udta for given service id
\param routedmx the ROUTE demultiplexer
\param service_id the target service
\return the user data associated with the service
 */
void *gf_route_dmx_get_service_udta(GF_ROUTEDmx *routedmx, u32 service_id);


/*! Patch fragment info of object after a repair
\param routedmx the ROUTE demultiplexer
\param service_id the target service
\param finfo file info event as passed to the caller. Only tsi and toi info are used to loacate the object. The frags and nb_frags fileds are updated by this function
\param br_start start offset of byte range being patched
\param br_end end offset of byte range being patched
\return error if any
 */
GF_Err gf_route_dmx_patch_frag_info(GF_ROUTEDmx *routedmx, u32 service_id, GF_ROUTEEventFileInfo *finfo, u32 br_start, u32 br_end);

/*! Patch object size after a repair - this might be needed by repair when the file size was not known
\param routedmx the ROUTE demultiplexer
\param service_id the target service
\param finfo file info event as passed to the caller. Only tsi and toi info are used to loacate the object. The frags and nb_frags fileds are updated by this function
\param new_size the new size to set
\return error if any
 */
GF_Err gf_route_dmx_patch_blob_size(GF_ROUTEDmx *routedmx, u32 service_id, GF_ROUTEEventFileInfo *finfo, u32 new_size);

/*! Set active status of a representation
\param routedmx the ROUTE demultiplexer
\param service_id the target service
\param period_id ID of the DASH period containing the representation, may be NULL
\param as_id ID of the DASH adaptation set containing the representation, may be 0
\param rep_id ID of the period containing the representation or HLS variant playlist URL, shall not be NULL
\param is_selected representation status
\return error if any
 */
GF_Err gf_route_dmx_mark_active_quality(GF_ROUTEDmx *routedmx, u32 service_id, const char *period_id, s32 as_id, const char *rep_id, Bool is_selected);

/*! Cancel all current transfer on  all services
\param routedmx the ROUTE demultiplexer
 */
void gf_route_dmx_reset_all(GF_ROUTEDmx *routedmx);


/*! @} */
#ifdef __cplusplus
}
#endif

#endif /* GPAC_DISABLE_ROUTE */

#endif	//_GF_ROUTE_H_

