/**
* Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

import vendor.qti.hardware.agm.CodecMediaConfig;
import vendor.qti.hardware.agm.CodecModulePayloadList;


@VintfStability
interface ICodecIpc {

    /**
     * Initialize codec interface.
     * Clients can't directly access this method as init is done in context of server
     * API is provided to keep in sync with native codec_interface.h
     */
	void ipc_cdc_interface_init();

    /**
     * De-initialize codec interface.
     * Clients can't directly access this method as init is done in context of server
     * API is provided to keep in sync with native codec_interface.h
     */
	void ipc_cdc_iterface_deinit();

    /**
     * Enables routing of codec endpoint
     * @param ep_id endpoint id
     * @param mpList list of routing payload for endpoint
     * @param set true to enable, false otherwise
     * @throws ServiceSpecificException with one of the values defined in Status.aidl
     * These exceptions are used to preserve the linux error codes over AIDL.
     * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
     */
	void ipc_cdc_route_endpoint(in int ep_id, in CodecModulePayloadList mpList, in boolean set);

    /**
     * Enables codec endpoint
     * @param ep_id end point id
     * @param mediaConfig media config for codec endpoint
     * @throws ServiceSpecificException with one of the values defined in Status.aidl
     * These exceptions are used to preserve the linux error codes over AIDL.
     * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
     */
	void ipc_cdc_enable_endpoint(in int ep_id, in CodecMediaConfig mediaConfig);

    /**
     * Disables codec endpoint
     * @param ep_id end point id
     * @throws ServiceSpecificException with one of the values defined in Status.aidl
     * These exceptions are used to preserve the linux error codes over AIDL.
     * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
     */
	void ipc_cdc_disable_endpoint(in int ep_id);

    /**
     * Set custom payload to codec endpoint
     * @param cdc_param_id param id supported by codec core
     * @param payload custom payload
     * @throws ServiceSpecificException with one of the values defined in Status.aidl
     * These exceptions are used to preserve the linux error codes over AIDL.
     * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
     */
	void ipc_cdc_set_custom_payload_endpoint(in int cdc_param_id, in byte[] payload);
}
