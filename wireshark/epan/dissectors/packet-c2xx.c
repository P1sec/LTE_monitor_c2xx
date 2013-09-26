#include "config.h"
#include <epan/packet.h>









static int proto_c2xx = -1;



/* Subtree handles: set by register_subtree_array */
static gint ett_c2xx = -1;
static gint ett_c2xx_header = -1;
static gint ett_c2xx_tag = -1;
static gint ett_c2xx_len = -1;
static gint ett_c2xx_sens = -1;
static gint ett_c2xx_ethdest = -1;
static gint ett_c2xx_ethsrc = -1;
static gint ett_c2xx_tag2 = -1;
static gint ett_c2xx_hdlc = -1;
static gint ett_c2xx_start = -1;
static gint ett_c2xx_len1 = -1;
static gint ett_c2xx_unk1 = -1;
static gint ett_c2xx_len2 = -1;
static gint ett_c2xx_trame = -1;
static gint ett_c2xx_end1 = -1;
static gint ett_c2xx_stamp = -1;
static gint ett_c2xx_maincommand = -1;
static gint ett_c2xx_subcommand = -1;
static gint ett_c2xx_lte = -1;
static gint ett_c2xx_hspa = -1;
static gint ett_c2xx_edge = -1;
static gint ett_c2xx_data = -1;
static gint ett_c2xx_stamp2 = -1;
static gint ett_c2xx_payload = -1;
static gint ett_c2xx_todo = -1;
static gint ett_c2xx_unk2 = -1;
//static gint ett_ipc = -1;



static int hf_c2xx = -1;

static int hf_c2xx_header = -1;
static int hf_c2xx_tag = -1;
static int hf_c2xx_len = -1;
static int hf_c2xx_sens = -1;
static int hf_c2xx_ethdest = -1;
static int hf_c2xx_ethsrc = -1;
static int hf_c2xx_tag2 = -1;
static int hf_c2xx_hdlc = -1;
static int hf_c2xx_start = -1;
static int hf_c2xx_len1 = -1;
static int hf_c2xx_unk1 = -1;
static int hf_c2xx_len2 = -1;
static int hf_c2xx_trame = -1;
static int hf_c2xx_end1 = -1;
static int hf_c2xx_stamp = -1;
static int hf_c2xx_maincommand = -1;
static int hf_c2xx_subcommand = -1;
static int hf_c2xx_lte = -1;
static int hf_c2xx_hspa = -1;
static int hf_c2xx_edge = -1;
static int hf_c2xx_data = -1;
static int hf_c2xx_stamp2 = -1;
static int hf_c2xx_payload = -1;
static int hf_c2xx_todo = -1;
static int hf_c2xx_unk2 = -1;







//static int hf_len = -1;
//static int hf_ipc = -1;


static dissector_handle_t eth_withoutfcs_handle;
static dissector_handle_t ip_handle;




// ===================================================================================================================


#define SAMSUNG_FRAME_START 0x57
#define SAMSUNG_IPC 0x43

enum SAMSUNG_MessageType {
  HIM =  0x1500,
  MTM,
  DM,
  CT
};

static const value_string SAMSUNG_MessageType_string[] =
{{ HIM, "HIM" },
{ MTM, "MTM" },
{ DM, "DM" },
{ CT, "CT" }};


enum SAMSUNG_DM_TransferType {
  API,
  Serial
};

static const value_string SAMSUNG_DM_TransferType_string[] =

  {
    { API, "API" },
    { Serial, "Serial" }

  };




#define SAMSUNG_HDLC_START 0x7f
#define SAMSUNG_HDLC_STOP 0x7e

enum SAMSUNG_IPC_MainCommand {
  IpcDmCmd = 0xa0,
  IpcCtCmd,
  IpcHimCmd
};


static const value_string SAMSUNG_IPC_MainCommand_string[] =
  {
    { IpcDmCmd, "IpcDmCmd" },
    { IpcCtCmd, "IpcCtCmd" },
    { IpcHimCmd, "IpcHimCmd" }  
  };

// CommonDataOutCommandType.cs
enum SAMSUNG_IPC_DM_CommonDataOutCommandType {
        BasicInformation = 0,
        CellInformation = 1,
        DataInformation = 2
};


static const value_string SAMSUNG_IPC_DM_CommonDataOutCommandType_string [] = {
       { BasicInformation, "BasicInformation" },
       { CellInformation, "CellInformation" },
       { DataInformation, "DataInformation" },
};


// ConnectionType.cs
enum SAMSUNG_IPC_DM_ConnectionType {
        Master = 0,
        Slave = 1
};


static const value_string SAMSUNG_IPC_DM_ConnectionType_string [] = {
       { Master, "Master" },
       { Slave, "Slave" },
};


// ControlCommandType.cs
enum SAMSUNG_IPC_DM_ControlCommandType {
        ChangeUpdatePeriodRequest = 6,
        ChangeUpdatePeriodResponse = 7,
        CommonItemRefreshRequest = 0x12,
        CommonItemRefreshResponse = 0x13,
        CommonItemSelectRequest = 0x10,
        CommonItemSelectResponse = 0x11,
        EdgeItemRefreshRequest = 50,
        EdgeItemRefreshResponse = 0x33,
        EdgeItemSelectRequest = 0x30,
        EdgeItemSelectResponse = 0x31,
        HspaItemRefreshRequest = 0x42,
        HspaItemRefreshResponse = 0x43,
        HspaItemSelectRequest = 0x40,
        HspaItemSelectResponse = 0x41,
        LteItemRefreshRequest = 0x22,
        LteItemRefreshResponse = 0x23,
        LteItemSelectRequest = 0x20,
        LteItemSelectResponse = 0x21,
        ResetRequest = 4,
        ResetResponse = 5,
        SleepRequest = 8,
        StartRequest = 0,
        StartResponse = 1,
        StopRequest = 2,
        StopResponse = 3,
        TraceItemSelectRequest = 80,
        TraceItemSelectResponse = 0x51,
        WakeupRequest = 9
};


static const value_string SAMSUNG_IPC_DM_ControlCommandType_string [] = {
       { ChangeUpdatePeriodRequest, "ChangeUpdatePeriodRequest" },
       { ChangeUpdatePeriodResponse, "ChangeUpdatePeriodResponse" },
       { CommonItemRefreshRequest, "CommonItemRefreshRequest" },
       { CommonItemRefreshResponse, "CommonItemRefreshResponse" },
       { CommonItemSelectRequest, "CommonItemSelectRequest" },
       { CommonItemSelectResponse, "CommonItemSelectResponse" },
       { EdgeItemRefreshRequest, "EdgeItemRefreshRequest" },
       { EdgeItemRefreshResponse, "EdgeItemRefreshResponse" },
       { EdgeItemSelectRequest, "EdgeItemSelectRequest" },
       { EdgeItemSelectResponse, "EdgeItemSelectResponse" },
       { HspaItemRefreshRequest, "HspaItemRefreshRequest" },
       { HspaItemRefreshResponse, "HspaItemRefreshResponse" },
       { HspaItemSelectRequest, "HspaItemSelectRequest" },
       { HspaItemSelectResponse, "HspaItemSelectResponse" },
       { LteItemRefreshRequest, "LteItemRefreshRequest" },
       { LteItemRefreshResponse, "LteItemRefreshResponse" },
       { LteItemSelectRequest, "LteItemSelectRequest" },
       { LteItemSelectResponse, "LteItemSelectResponse" },
       { ResetRequest, "ResetRequest" },
       { ResetResponse, "ResetResponse" },
       { SleepRequest, "SleepRequest" },
       { StartRequest, "StartRequest" },
       { StartResponse, "StartResponse" },
       { StopRequest, "StopRequest" },
       { StopResponse, "StopResponse" },
       { TraceItemSelectRequest, "TraceItemSelectRequest" },
       { TraceItemSelectResponse, "TraceItemSelectResponse" },
       { WakeupRequest, "WakeupRequest" },
};


// EdgeDataOutCommandType.cs
enum SAMSUNG_IPC_DM_EdgeDataOutCommandType {
        Phy3GNCellInfo = 7,
        Phy4GNCellInfo = 13,
        PhyBasicInfo = 10,
        PhyHandoverHistoryInfo = 9,
        PhyHandoverInfo = 8,
        PhyMeasurementInfo = 11,
        PhyMmGmmInfo = 0x11,
        PhyModulationMeasureReportInfo = 3,
        PhyNCellInfo = 6,
        PhyPowerControlInfo = 12,
        PhyQoSInfo = 0x10,
        PhyRlcInfo = 1,
        PhySCellInfo = 5,
        PhyTimeSlotInfo = 0
};


static const value_string SAMSUNG_IPC_DM_EdgeDataOutCommandType_string [] = {
       { Phy3GNCellInfo, "Phy3GNCellInfo" },
       { Phy4GNCellInfo, "Phy4GNCellInfo" },
       { PhyBasicInfo, "PhyBasicInfo" },
       { PhyHandoverHistoryInfo, "PhyHandoverHistoryInfo" },
       { PhyHandoverInfo, "PhyHandoverInfo" },
       { PhyMeasurementInfo, "PhyMeasurementInfo" },
       { PhyMmGmmInfo, "PhyMmGmmInfo" },
       { PhyModulationMeasureReportInfo, "PhyModulationMeasureReportInfo" },
       { PhyNCellInfo, "PhyNCellInfo" },
       { PhyPowerControlInfo, "PhyPowerControlInfo" },
       { PhyQoSInfo, "PhyQoSInfo" },
       { PhyRlcInfo, "PhyRlcInfo" },
       { PhySCellInfo, "PhySCellInfo" },
       { PhyTimeSlotInfo, "PhyTimeSlotInfo" },
};


// HspaDataOutCommandType.cs
enum SAMSUNG_IPC_DM_HspaDataOutCommandType {
        GDDpaInfo = 3,
        GDDpaInfo2 = 4,
        GDDpaTxInfo = 5,
        GPFingerInfo = 2,
        GPPowerControlInfo = 0,
        GPTrchBlerInfo = 1,
        UL1CellMeasurementInfo = 0x17,
        UL1DpchPowerInfo = 0x15,
        UL1EutraMeasurementInfo = 30,
        UL1FrequencySearchInfo = 0x12,
        UL1GsmMeasurementInfo = 0x19,
        UL1InterfreqCellReselectionInfo = 0x1d,
        UL1InterFreqMeasurementInfo = 0x18,
        UL1InternalMeasurementInfo = 0x1a,
        UL1IntrafreqCellReselectionInfo = 0x1c,
        UL1MidTypeInfo = 0x16,
        UL1OlpcInfo = 20,
        UL1PowerControlInfo = 0x13,
        UL1SearchInfo = 0x11,
        UL1ServingCellMeasurementInfo = 0x1b,
        UL1UmtsRfInfo = 0x10,
        UL2EulMacInfo = 0x38,
        UL2HsMac2Info = 0x39,
        UL2HsMacEhsInfo = 0x3b,
        UL2HsMacInfo = 0x37,
        UL2HspaMmGmmInfo = 0x3a,
        UL2RlcAmChannelStaticsInfo = 0x31,
        UL2RlcUmChannelStaticsInfo = 0x33,
        UL2UdpchChannelConfigInfo = 0x30,
        UL2UrlcAmConfigInfo = 50,
        UL2UrlcTmConfigInfo = 0x35,
        UL2UrlcUmConfigInfo = 0x34,
        UL2WcdmaMacInfo = 0x36,
        UrrcNetworkInfo = 0x22,
        UrrcRbMappingInfo = 0x21,
        UrrcStatusInfo = 0x20,
        UulPowerInfo = 0x2a,
        UulRachConfigInfo = 40,
        UulUdpchConfigInfo = 0x29
};


static const value_string SAMSUNG_IPC_DM_HspaDataOutCommandType_string [] = {
       { GDDpaInfo, "GDDpaInfo" },
       { GDDpaInfo2, "GDDpaInfo2" },
       { GDDpaTxInfo, "GDDpaTxInfo" },
       { GPFingerInfo, "GPFingerInfo" },
       { GPPowerControlInfo, "GPPowerControlInfo" },
       { GPTrchBlerInfo, "GPTrchBlerInfo" },
       { UL1CellMeasurementInfo, "UL1CellMeasurementInfo" },
       { UL1DpchPowerInfo, "UL1DpchPowerInfo" },
       { UL1EutraMeasurementInfo, "UL1EutraMeasurementInfo" },
       { UL1FrequencySearchInfo, "UL1FrequencySearchInfo" },
       { UL1GsmMeasurementInfo, "UL1GsmMeasurementInfo" },
       { UL1InterfreqCellReselectionInfo, "UL1InterfreqCellReselectionInfo" },
       { UL1InterFreqMeasurementInfo, "UL1InterFreqMeasurementInfo" },
       { UL1InternalMeasurementInfo, "UL1InternalMeasurementInfo" },
       { UL1IntrafreqCellReselectionInfo, "UL1IntrafreqCellReselectionInfo" },
       { UL1MidTypeInfo, "UL1MidTypeInfo" },
       { UL1OlpcInfo, "UL1OlpcInfo" },
       { UL1PowerControlInfo, "UL1PowerControlInfo" },
       { UL1SearchInfo, "UL1SearchInfo" },
       { UL1ServingCellMeasurementInfo, "UL1ServingCellMeasurementInfo" },
       { UL1UmtsRfInfo, "UL1UmtsRfInfo" },
       { UL2EulMacInfo, "UL2EulMacInfo" },
       { UL2HsMac2Info, "UL2HsMac2Info" },
       { UL2HsMacEhsInfo, "UL2HsMacEhsInfo" },
       { UL2HsMacInfo, "UL2HsMacInfo" },
       { UL2HspaMmGmmInfo, "UL2HspaMmGmmInfo" },
       { UL2RlcAmChannelStaticsInfo, "UL2RlcAmChannelStaticsInfo" },
       { UL2RlcUmChannelStaticsInfo, "UL2RlcUmChannelStaticsInfo" },
       { UL2UdpchChannelConfigInfo, "UL2UdpchChannelConfigInfo" },
       { UL2UrlcAmConfigInfo, "UL2UrlcAmConfigInfo" },
       { UL2UrlcTmConfigInfo, "UL2UrlcTmConfigInfo" },
       { UL2UrlcUmConfigInfo, "UL2UrlcUmConfigInfo" },
       { UL2WcdmaMacInfo, "UL2WcdmaMacInfo" },
       { UrrcNetworkInfo, "UrrcNetworkInfo" },
       { UrrcRbMappingInfo, "UrrcRbMappingInfo" },
       { UrrcStatusInfo, "UrrcStatusInfo" },
       { UulPowerInfo, "UulPowerInfo" },
       { UulRachConfigInfo, "UulRachConfigInfo" },
       { UulUdpchConfigInfo, "UulUdpchConfigInfo" },
};


// ItemStatus.cs
enum SAMSUNG_IPC_DM_ItemStatus {
        Off = 0,
        On = 1
};


static const value_string SAMSUNG_IPC_DM_ItemStatus_string [] = {
       { Off, "Off" },
       { On, "On" },
};


// LteDataOutCommandType.cs
enum SAMSUNG_IPC_DM_LteDataOutCommandType {
        DataThroughputInfo =    96, // 0x60,
        DataTimingInfo =        97, // 0x61,
        L1DownlinkInfo =        18, // 0x12,
        L1MeasurementConfig =   24, // 0x18,
        L1RfInfo =              16, // 0x10,
        L1SyncInfo =            17, // 0x11,
        L1UplinkInfo =          19, // 0x13,
        L2DlSchConfig =         49, // 0x31,
        L2MaxHarqMsg3Tx =       56, // 0x38,
        L2PdcpDlInfo =          67, // 0x43,
        L2PdcpUlInfo =          66, // 0x42,
        L2PhrConfig =           52, // 0x34,
        L2PowerRampingStep =    54, // 0x36,
        L2PreambleInfo =        53, // 0x35,
        L2RachInfo =            57, // 0x39,
        L2RaSupervisionInfo =   55, // 0x37,
        L2RbInfo =              64, // 0x40,
        L2RlcStatusInfo =       65, // 0x41,
        L2RntiInfo =            58, // 0x3a,
        L2TimeAlignmentTimer =  51, // 0x33,
        L2UlSchConfig =         50, // 0x32
        L2UlSpecificParam =     48, // 0x30,
        L2UlSyncStatInfo =      60, // 0x3C
        NasIPInfo =             94, // 0x5e,
        NasL3MmMsgInfo =        90, // 0x5a
        NasL3SmMsgInfo =        95, // 0x5f, 
        NasPdpInfo =            93, // 0x5d,
        NasPlmnSelectionInfo =  91, // 0x5b,
        NasSecurityInfo =       92, // 0x5c,
        NasSimDataInfo =        88, // 0x58,
        NasStateVariableInfo =  89, // 0x59,
        PhyCellSearchMeasInfo =  1, // 0x01,
        PhyChanQualInfo =        5, // 0x05
        PhyParameterInfo =       6, // 0x06
        PhyPhichInfo =           7, // 0x07
        PhyPhyStatusInfo =       0, // 0x00
        PhySystemInfo =          4, // 0x04
        RrcAsnVerInfo =         84, // 0x54,
        RrcPeerMsgInfo =        82, // 0x52,
        RrcServingCellInfo =    80, // 0x50
        RrcStatusVariableInfo = 81, // 0x51,
        RrcTimerInfo =          83, // 0x53
};


static const value_string SAMSUNG_IPC_DM_LteDataOutCommandType_string [] = {
       { DataThroughputInfo, "DataThroughputInfo" },
       { DataTimingInfo, "DataTimingInfo" },
       { L1DownlinkInfo, "L1DownlinkInfo" },
       { L1MeasurementConfig, "L1MeasurementConfig" },
       { L1RfInfo, "L1RfInfo" },
       { L1SyncInfo, "L1SyncInfo" },
       { L1UplinkInfo, "L1UplinkInfo" },
       { L2DlSchConfig, "L2DlSchConfig" },
       { L2MaxHarqMsg3Tx, "L2MaxHarqMsg3Tx" },
       { L2PdcpDlInfo, "L2PdcpDlInfo" },
       { L2PdcpUlInfo, "L2PdcpUlInfo" },
       { L2PhrConfig, "L2PhrConfig" },
       { L2PowerRampingStep, "L2PowerRampingStep" },
       { L2PreambleInfo, "L2PreambleInfo" },
       { L2RachInfo, "L2RachInfo" },
       { L2RaSupervisionInfo, "L2RaSupervisionInfo" },
       { L2RbInfo, "L2RbInfo" },
       { L2RlcStatusInfo, "L2RlcStatusInfo" },
       { L2RntiInfo, "L2RntiInfo" },
       { L2TimeAlignmentTimer, "L2TimeAlignmentTimer" },
       { L2UlSchConfig, "L2UlSchConfig" },
       { L2UlSpecificParam, "L2UlSpecificParam" },
       { L2UlSyncStatInfo, "L2UlSyncStatInfo" },
       { NasIPInfo, "NasIPInfo" },
       { NasL3MmMsgInfo, "NasL3MmMsgInfo" },
       { NasL3SmMsgInfo, "NasL3SmMsgInfo" },
       { NasPdpInfo, "NasPdpInfo" },
       { NasPlmnSelectionInfo, "NasPlmnSelectionInfo" },
       { NasSecurityInfo, "NasSecurityInfo" },
       { NasSimDataInfo, "NasSimDataInfo" },
       { NasStateVariableInfo, "NasStateVariableInfo" },
       { PhyCellSearchMeasInfo, "PhyCellSearchMeasInfo" },
       { PhyChanQualInfo, "PhyChanQualInfo" },
       { PhyParameterInfo, "PhyParameterInfo" },
       { PhyPhichInfo, "PhyPhichInfo" },
       { PhyPhyStatusInfo, "PhyPhyStatusInfo" },
       { PhySystemInfo, "PhySystemInfo" },
       { RrcAsnVerInfo, "RrcAsnVerInfo" },
       { RrcPeerMsgInfo, "RrcPeerMsgInfo" },
       { RrcServingCellInfo, "RrcServingCellInfo" },
       { RrcStatusVariableInfo, "RrcStatusVariableInfo" },
       { RrcTimerInfo, "RrcTimerInfo" },
};


// NumberOfItemStatus.cs
enum SAMSUNG_IPC_DM_NumberOfItemStatus {
        AllDisabled = 0,
        AllEnabled = 0xff
};


static const value_string SAMSUNG_IPC_DM_NumberOfItemStatus_string [] = {
       { AllDisabled, "AllDisabled" },
       { AllEnabled, "AllEnabled" },
};


// Rat.cs
enum SAMSUNG_IPC_DM_Rat {
        Edge = 3,
        Hedge = 4,
        Hspa = 2,
        Lte = 1,
        LteSdr = 0x10,
        Na = 0
};


static const value_string SAMSUNG_IPC_DM_Rat_string [] = {
       { Edge, "Edge" },
       { Hedge, "Hedge" },
       { Hspa, "Hspa" },
       { Lte, "Lte" },
       { LteSdr, "LteSdr" },
       { Na, "Na" },
};


// SubCommand.cs
enum SAMSUNG_IPC_DM_SubCommand {
        ControlMsg = 0,
        CommonDataOut = 1,
        LteDataOut = 2,
        EdgeDataOut = 3,
        HspaDataOut = 4,
        TraceDataOut = 5
};


static const value_string SAMSUNG_IPC_DM_SubCommand_string [] = {
       { ControlMsg, "ControlMsg" },
       { CommonDataOut, "CommonDataOut" },
       { LteDataOut, "LteDataOut" },
       { EdgeDataOut, "EdgeDataOut" },
       { HspaDataOut, "HspaDataOut" },
       { TraceDataOut, "TraceDataOut" },
};


// TraceDataOutCommandType.cs
enum SAMSUNG_IPC_DM_TraceDataOutCommandType {
        LteL1 = 0,
        LteL2 = 1,
        LteRrc = 2,
        LteSmc = 3
};


static const value_string SAMSUNG_IPC_DM_TraceDataOutCommandType_string [] = {
       { LteL1, "LteL1" },
       { LteL2, "LteL2" },
       { LteRrc, "LteRrc" },
       { LteSmc, "LteSmc" },
};


// UpdatePeriod.cs
enum SAMSUNG_IPC_DM_UpdatePeriod {
        ms100 = 2,
        ms1000 = 5,
        ms10000 = 8,
        ms20 = 0,
        ms200 = 3,
        ms2000 = 6,
        ms50 = 1,
        ms500 = 4,
        ms5000 = 7
};


static const value_string SAMSUNG_IPC_DM_UpdatePeriod_string [] = {
       { ms100, "ms100" },
       { ms1000, "ms1000" },
       { ms10000, "ms10000" },
       { ms20, "ms20" },
       { ms200, "ms200" },
       { ms2000, "ms2000" },
       { ms50, "ms50" },
       { ms500, "ms500" },
       { ms5000, "ms5000" },
};





// =====================================================================================================================
static void dissect_c2xx_ctrl(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  proto_item *c2xx_item;
  proto_item *c2xx_sub_item; 
  proto_tree *c2xx_tree; 
  proto_tree *c2xx_header_tree;
  proto_tree *c2xx_hdlc_tree;
  proto_tree *c2xx_trame_tree;
  proto_tree *c2xx_data_tree;
  proto_tree *c2xx_payload_tree;
  int subcommand;

  guint8 val, val2, val3;

  //nstime_t tv;
  

  gint len;
  int offset = 0;
  

  len = tvb_reported_length(tvb);
  col_append_str(pinfo->cinfo, COL_PROTOCOL, "/C2C");
  col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "C2XX");

  if(tree) {

    c2xx_item = proto_tree_add_item(tree, proto_c2xx, tvb, 0, -1, FALSE);
    c2xx_tree = proto_item_add_subtree(c2xx_item, ett_c2xx);
    
    c2xx_header_tree = proto_item_add_subtree(c2xx_item, ett_c2xx);
    c2xx_sub_item = proto_tree_add_item( c2xx_header_tree, hf_c2xx_header, tvb, offset, 20, ENC_NA );
    c2xx_header_tree = proto_item_add_subtree(c2xx_sub_item, ett_c2xx);

    proto_tree_add_item( c2xx_header_tree, hf_c2xx_tag, tvb, offset, 2, ENC_BIG_ENDIAN );
    offset += 2;

    proto_tree_add_item( c2xx_header_tree, hf_c2xx_len, tvb, offset, 2, ENC_LITTLE_ENDIAN );
    offset += 2;

    proto_tree_add_item( c2xx_header_tree, hf_c2xx_sens, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 2;

    proto_tree_add_item( c2xx_header_tree, hf_c2xx_ethdest, tvb, offset, 6, ENC_NA);
    offset += 6;

    proto_tree_add_item( c2xx_header_tree, hf_c2xx_ethsrc, tvb, offset, 6, ENC_NA);
    offset += 6;

    proto_tree_add_item( c2xx_header_tree, hf_c2xx_tag2, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 2;

    c2xx_sub_item = proto_tree_add_item( c2xx_tree, hf_c2xx_hdlc, tvb, offset, len-20, FALSE );
    c2xx_hdlc_tree = proto_item_add_subtree(c2xx_sub_item, ett_c2xx);

    proto_tree_add_item( c2xx_hdlc_tree, hf_c2xx_start, tvb, offset, 1, ENC_NA );
    offset +=1;

    proto_tree_add_item( c2xx_hdlc_tree, hf_c2xx_len1, tvb, offset, 2, ENC_LITTLE_ENDIAN );
    offset +=2;

    proto_tree_add_item( c2xx_hdlc_tree, hf_c2xx_unk1, tvb, offset, 1, ENC_NA );
    offset +=1;

    proto_tree_add_item( c2xx_hdlc_tree, hf_c2xx_len2, tvb, offset, 2, ENC_LITTLE_ENDIAN );
    offset +=2;

    c2xx_sub_item = proto_tree_add_item( c2xx_hdlc_tree, hf_c2xx_trame, tvb, offset, len-offset-1, FALSE );
    c2xx_trame_tree = proto_item_add_subtree(c2xx_sub_item, ett_c2xx_trame);

    proto_tree_add_item( c2xx_trame_tree, hf_c2xx_stamp, tvb, offset, 2, ENC_NA );
    offset +=2;

    proto_tree_add_item( c2xx_trame_tree, hf_c2xx_maincommand, tvb, offset, 1, ENC_NA );
    offset +=1;

    /*
        BasicInformation = 0,
        CellInformation = 1,
        DataInformation = 2
    */
    proto_tree_add_item( c2xx_trame_tree, hf_c2xx_subcommand, tvb, offset, 1, ENC_NA );
    offset +=1;

    // variable suivant index
    //  LteDataOut 2
    subcommand = tvb_get_guint8(tvb, 0x1D);
    switch(subcommand) {
      case LteDataOut:
        proto_tree_add_item( c2xx_trame_tree, hf_c2xx_lte, tvb, offset, 1, ENC_NA );
        col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "Lte"); 
        break;
      case HspaDataOut:
        proto_tree_add_item( c2xx_trame_tree, hf_c2xx_hspa, tvb, offset, 1, ENC_NA );
        col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "Hspa");
        break;
      case EdgeDataOut:
        proto_tree_add_item( c2xx_trame_tree, hf_c2xx_edge, tvb, offset, 1, ENC_NA );
        col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "Edge");
        break;      
      default:
        break;
    }
    offset +=1;

    if(len-offset-1 > 0) {
      c2xx_sub_item = proto_tree_add_item( c2xx_trame_tree, hf_c2xx_data, tvb, offset, len-offset-1, FALSE );
      c2xx_data_tree = proto_item_add_subtree(c2xx_sub_item, ett_c2xx_data);

      proto_tree_add_item( c2xx_data_tree, hf_c2xx_stamp2, tvb, offset, 4, ENC_LITTLE_ENDIAN );
      offset +=4;

      c2xx_sub_item = proto_tree_add_item( c2xx_data_tree, hf_c2xx_payload, tvb, offset, len-offset-1, FALSE );
      c2xx_payload_tree = proto_item_add_subtree(c2xx_sub_item, ett_c2xx_payload);

      switch(subcommand) {
        case CommonDataOut:
          val = tvb_get_guint8(tvb, offset);
          val2 = tvb_get_guint8(tvb, offset+1);
          val3 = tvb_get_guint8(tvb, offset+2);

          // XX YY ZZ ll ll
          // 30 
          //    30-31          30 ccch 31 dcch  32 bcch
          //       01/02       01 ul  02 dl
          //          ll ll    length

          if(val == 0x30) {
            if(val2 == 0x30) {
              if(val3 == 0x01) { 
                // 30 30 01     3505 home.pcap
                offset += 5;
                col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "rrc.ul.ccch : ");
                call_dissector(find_dissector("rrc.ul.ccch"), tvb_new_subset(tvb, offset, len-offset-1, -1), pinfo, tree);                
              } else if(val3 == 0x02) { 
                // 30 30 02     3538
                offset += 5;
                col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "rrc.dl.ccch : ");
                call_dissector(find_dissector("rrc.dl.ccch"), tvb_new_subset(tvb, offset, len-offset-1, -1), pinfo, tree);                                
              }
            } else if(val2 == 0x32) {
              // 30 32 (02)     3506
              offset +=5;
              col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "rrc.bcch.bch : ");
              call_dissector(find_dissector("rrc.bcch.bch"), tvb_new_subset(tvb, offset, len-offset-1, -1), pinfo, tree);  
            } else if(val2 == 0x31) { 
              if(val3==0x01) {
                // 30 31 01     3541
                offset +=5;
                col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "rrc.ul.dcch : ");
                call_dissector(find_dissector("rrc.ul.dcch"), tvb_new_subset(tvb, offset, len-offset-1, -1), pinfo, tree);                   
              } else if(val3 == 0x02) {
                // 30 31 02     3602
                offset +=5;
                col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "rrc.dl.dcch : ");
                call_dissector(find_dissector("rrc.dl.dcch"), tvb_new_subset(tvb, offset, len-offset-1, -1), pinfo, tree);   
              }
            }

            /*
            packet-rrc.c
              new_register_dissector("rrc.dl.dcch", dissect_DL_DCCH_Message_PDU, proto_rrc);
              new_register_dissector("rrc.ul.dcch", dissect_UL_DCCH_Message_PDU, proto_rrc);
              new_register_dissector("rrc.dl.ccch", dissect_DL_CCCH_Message_PDU, proto_rrc);
              new_register_dissector("rrc.ul.ccch", dissect_UL_CCCH_Message_PDU, proto_rrc);
              new_register_dissector("rrc.pcch", dissect_PCCH_Message_PDU, proto_rrc);
              new_register_dissector("rrc.dl.shcch", dissect_DL_SHCCH_Message_PDU, proto_rrc);
              new_register_dissector("rrc.ul.shcch", dissect_UL_SHCCH_Message_PDU, proto_rrc);
              new_register_dissector("rrc.bcch.fach", dissect_BCCH_FACH_Message_PDU, proto_rrc);
              new_register_dissector("rrc.bcch.bch", dissect_BCCH_BCH_Message_PDU, proto_rrc);
              new_register_dissector("rrc.mcch", dissect_MCCH_Message_PDU, proto_rrc);
              new_register_dissector("rrc.msch", dissect_MSCH_Message_PDU, proto_rrc);
              new_register_dissector("rrc.irat.ho_to_utran_cmd", dissect_rrc_HandoverToUTRANCommand_PDU, proto_rrc);
              new_register_dissector("rrc.irat.irat_ho_info", dissect_rrc_InterRATHandoverInfo_PDU, proto_rrc);
              new_register_dissector("rrc.sysinfo", dissect_SystemInformation_BCH_PDU, proto_rrc);
              new_register_dissector("rrc.sysinfo.cont", dissect_System_Information_Container_PDU, proto_rrc);
              new_register_dissector("rrc.ue_radio_access_cap_info", dissect_UE_RadioAccessCapabilityInfo_PDU, proto_rrc);
              new_register_dissector("rrc.si.mib", dissect_rrc_MasterInformationBlock_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib1", dissect_rrc_SysInfoType1_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib2", dissect_rrc_SysInfoType2_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib3", dissect_rrc_SysInfoType3_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib4", dissect_SysInfoType4_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib5", dissect_SysInfoType5_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib5bis", dissect_SysInfoType5bis_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib6", dissect_SysInfoType6_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib7", dissect_rrc_SysInfoType7_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib8", dissect_SysInfoType8_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib9", dissect_SysInfoType9_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib10", dissect_SysInfoType10_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib11", dissect_SysInfoType11_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib11bis", dissect_SysInfoType11bis_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib12", dissect_rrc_SysInfoType12_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib13", dissect_SysInfoType13_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib13-1", dissect_SysInfoType13_1_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib13-2", dissect_SysInfoType13_2_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib13-3", dissect_SysInfoType13_3_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib13-4", dissect_SysInfoType13_4_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib14", dissect_SysInfoType14_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib15", dissect_SysInfoType15_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib15bis", dissect_SysInfoType15bis_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib15-1", dissect_SysInfoType15_1_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib15-1bis", dissect_SysInfoType15_1bis_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib15-2", dissect_SysInfoType15_2_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib15-2bis", dissect_SysInfoType15_2bis_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib15-2ter", dissect_SysInfoType15_2ter_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib15-3", dissect_SysInfoType15_3_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib15-3bis", dissect_SysInfoType15_3bis_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib15-4", dissect_SysInfoType15_4_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib15-5", dissect_SysInfoType15_5_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib15-6", dissect_SysInfoType15_6_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib15-7", dissect_SysInfoType15_7_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib15-8", dissect_SysInfoType15_8_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib16", dissect_SysInfoType16_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib17", dissect_SysInfoType17_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib18", dissect_SysInfoType18_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib19", dissect_SysInfoType19_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib20", dissect_SysInfoType20_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib21", dissect_SysInfoType21_PDU, proto_rrc);
              new_register_dissector("rrc.si.sib22", dissect_SysInfoType22_PDU, proto_rrc);
              new_register_dissector("rrc.si.sb1", dissect_SysInfoTypeSB1_PDU, proto_rrc);
              new_register_dissector("rrc.si.sb2", dissect_SysInfoTypeSB2_PDU, proto_rrc);
              new_register_dissector("rrc.s_to_trnc_cont", dissect_rrc_ToTargetRNC_Container_PDU, proto_rrc);
              new_register_dissector("rrc.t_to_srnc_cont", dissect_rrc_TargetRNC_ToSourceRNC_Container_PDU, proto_rrc);
            */

          }
          break;

        case LteDataOut:
          val = tvb_get_guint8(tvb, 0x1E);
          switch(val) {
            case RrcPeerMsgInfo:
              proto_tree_add_item( c2xx_payload_tree, hf_c2xx_unk2, tvb, offset, 4, ENC_LITTLE_ENDIAN );
              offset +=4;

              val2 = tvb_get_guint8(tvb, 0x23);
              switch(val2) {
                case 0:
                  SET_ADDRESS(&pinfo->src, AT_STRINGZ, 3, "UE");
                  SET_ADDRESS(&pinfo->dst, AT_STRINGZ, 6, "eNodeB");
                  col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "lte-rrc.ul.ccch : ");
                  call_dissector(find_dissector("lte-rrc.ul.ccch"), tvb_new_subset(tvb, offset, len-offset-1, -1), pinfo, tree);

                  break;
                case 1:
                  // 01:00:09:00    40:00:3c:43:a0:07:f0:00:00:7e
                  //  Channel_PCCH
                  col_clear(pinfo->cinfo, COL_PROTOCOL);
                  call_dissector(find_dissector("lte-rrc.pcch"), tvb_new_subset(tvb, offset, len-offset-1, -1), pinfo, tree);
                  break;
                case 3:
                  // 03:00:12:00    40:48:20:03:c9:81:17:2d:b0:38:1d:31:00:81:84:6c:24:6a:7e
                  //  Channel_BCCH
                  //    DLSCH_TRANSPORT
                  SET_ADDRESS(&pinfo->src, AT_STRINGZ, 6, "eNodeB");
                  SET_ADDRESS(&pinfo->dst, AT_STRINGZ, 3, "UE");
                  col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "lte-rrc.bcch.dl.sch : ");
                  call_dissector(find_dissector("lte-rrc.bcch.dl.sch"), tvb_new_subset(tvb, offset, len-offset-1, -1), pinfo, tree);
                  break;
                case 4:
                  // 04 lte-rrc.dl.dcch
                  SET_ADDRESS(&pinfo->src, AT_STRINGZ, 6, "eNodeB");
                  SET_ADDRESS(&pinfo->dst, AT_STRINGZ, 3, "UE");
                  col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "lte-rrc.dl.dcch : ");
                  call_dissector(find_dissector("lte-rrc.dl.dcch"), tvb_new_subset(tvb, offset, len-offset-1, -1), pinfo, tree);
                  break;
                default:
                  break;
              }

              /*
              https://www.wireshark.org/lists/wireshark-users/201002/msg00198.html

                new_register_dissector("lte-rrc.bcch.bch", dissect_BCCH_BCH_Message_PDU, proto_lte_rrc);
                new_register_dissector("lte-rrc.bcch.dl.sch", dissect_BCCH_DL_SCH_Message_PDU, proto_lte_rrc);
                new_register_dissector("lte-rrc.mcch", dissect_MCCH_Message_PDU, proto_lte_rrc);
                new_register_dissector("lte-rrc.pcch", dissect_PCCH_Message_PDU, proto_lte_rrc);
                new_register_dissector("lte-rrc.dl.ccch", dissect_DL_CCCH_Message_PDU, proto_lte_rrc);
                new_register_dissector("lte-rrc.dl.dcch", dissect_DL_DCCH_Message_PDU, proto_lte_rrc);
                new_register_dissector("lte-rrc.ul.ccch", dissect_UL_CCCH_Message_PDU, proto_lte_rrc);
                new_register_dissector("lte-rrc.ul.dcch", dissect_UL_DCCH_Message_PDU, proto_lte_rrc);
                new_register_dissector("lte-rrc.ue_cap_info", dissect_UECapabilityInformation_PDU, proto_lte_rrc);
                new_register_dissector("lte-rrc.ue_eutra_cap", dissect_lte_rrc_UE_EUTRA_Capability_PDU, proto_lte_rrc);

              */
              break;

            // lte4g.pcap 511 NasIPInfo
            //  NOK eps_plain 
            
            // lte4g.pcap 465 NasSecurityInfo
            //  NOK nas-eps_plain 
            //  NOK lte-rrc.bcch.bch lte-rrc.bcch.dl.sch lte-rrc.mcch lte-rrc.pcch lte-rrc.dl.ccch lte-rrc.dl.dcch lte-rrc.ul.ccch lte-rrc.ul.dcch
            //  NOK lte-rrc.ue_cap_info lte-rrc.ue_eutra_cap
            /*
            case NasSecurityInfo:
              proto_tree_add_item( c2xx_payload_tree, hf_c2xx_unk2, tvb, offset, 4, ENC_LITTLE_ENDIAN );
              offset +=4;
              call_dissector(find_dissector(""), tvb_new_subset(tvb, offset, len-offset-1, -1), pinfo, tree);
              break;
            */

            case NasL3SmMsgInfo:
            case NasL3MmMsgInfo:
              proto_tree_add_item( c2xx_payload_tree, hf_c2xx_unk2, tvb, offset, 4, ENC_LITTLE_ENDIAN );
              offset +=4;
              val2 = tvb_get_guint8(tvb, 0x23);
              switch(val2) {
                case 0:
                  SET_ADDRESS(&pinfo->src, AT_STRINGZ, 6, "eNodeB");
                  SET_ADDRESS(&pinfo->dst, AT_STRINGZ, 3, "UE");
                  break;
                case 1:
                  SET_ADDRESS(&pinfo->src, AT_STRINGZ, 3, "UE");
                  SET_ADDRESS(&pinfo->dst, AT_STRINGZ, 6, "eNodeB");
                  break;
              }
              col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "nas-eps_plain: ");
              call_dissector(find_dissector("nas-eps_plain"), tvb_new_subset(tvb, offset, len-offset-1, -1), pinfo, tree);
              break;

            default:
              proto_tree_add_item( c2xx_payload_tree, hf_c2xx_todo, tvb, offset, len-offset-1, ENC_NA );          
              break;
          }

          break;

        default:
          break;

      }

    }

    proto_tree_add_item( c2xx_hdlc_tree, hf_c2xx_end1, tvb, len-1, 1, FALSE );

  }
}


static void dissect_c2xx_data(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  proto_item *item;
  proto_tree *c2xx_tree;
//  gint len;
  int offset = 0;
//  tvbuff_t * next_tvb;


//  len = tvb_reported_length(tvb);
  col_append_str(pinfo->cinfo, COL_PROTOCOL, "/DATA");
  col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "DATA");

  if(tree) {
    item = proto_tree_add_item(tree, proto_c2xx, tvb, 0, 6, ENC_NA);
    c2xx_tree = proto_item_add_subtree(item, ett_c2xx);
    offset += 6;

// Ethernet II - bien sauf que les trames depassent 1400
    call_dissector(eth_withoutfcs_handle, tvb_new_subset_remaining(tvb, offset), pinfo, tree);

// ou IP directement
/*
    offset += 6;                                       // dest mac
    offset += 6;                                       // src  mac
    offset += 2;                                       // ether type
    next_tvb = tvb_new_subset_remaining(tvb, offset);
    call_dissector(ip_handle,next_tvb,pinfo,tree);
*/
  }
}



/* The dissector itself */
static void dissect_c2xx(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  if(!tvb_memeql(tvb,1,"\x43", 1)){
    dissect_c2xx_ctrl(tvb, pinfo, tree);
  } else {
    dissect_c2xx_data(tvb, pinfo, tree);
  }
}



void
proto_register_c2xx(void)
{
  static hf_register_info hf[] = {
    { &hf_c2xx,
      { "C2XX Samsung Protocol", "c2xx.data", FT_NONE, BASE_NONE, NULL, 0x0, 
            NULL, HFILL }},
    { &hf_c2xx_header,
        { "Header", "c2xx.header", FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
    { &hf_c2xx_tag,
        { "Tag", "c2xx.tag", FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_c2xx_len,
        { "Len", "c2xx.len", FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_c2xx_sens,
        { "Sens", "c2xx.sens", FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_c2xx_ethdest,
        { "Eth dest", "c2xx.ethdest", FT_ETHER, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},            
    { &hf_c2xx_ethsrc,
        { "Eth src", "c2xx.ethsrc", FT_ETHER, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},            
    { &hf_c2xx_tag2,
        { "Tag2", "c2xx.tag2", FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_c2xx_hdlc,
        { "Hdlc", "c2xx.hdlc", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
    { &hf_c2xx_start,
        { "Start", "c2xx.start", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
    { &hf_c2xx_len1,
        { "Len1", "c2xx.len1", FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_c2xx_unk1,
        { "Unk1", "c2xx.unk1", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
    { &hf_c2xx_len2,
        { "Len2", "c2xx.len2", FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_c2xx_trame,
        { "Trame", "c2xx.trame", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
    { &hf_c2xx_end1,
        { "End1", "c2xx.end1", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
    { &hf_c2xx_stamp,
        { "Stamp", "c2xx.stamp", FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_c2xx_maincommand,
        { "MainCommand", "c2xx.maincommand", FT_UINT8, BASE_DEC, VALS(SAMSUNG_IPC_MainCommand_string), 0x0,
            NULL, HFILL }},
    { &hf_c2xx_subcommand,
        { "SubCommand", "c2xx.subcommand", FT_UINT8, BASE_DEC, VALS(SAMSUNG_IPC_DM_SubCommand_string), 0x0,
            NULL, HFILL }},
    { &hf_c2xx_lte,
        { "LteDataOutCommand", "c2xx.lte", FT_UINT8, BASE_DEC, VALS(SAMSUNG_IPC_DM_LteDataOutCommandType_string), 0x0,
            NULL, HFILL }},
    { &hf_c2xx_hspa,
        { "HspaDataOutCommand", "c2xx.hspa", FT_UINT8, BASE_DEC, VALS(SAMSUNG_IPC_DM_HspaDataOutCommandType_string), 0x0,
            NULL, HFILL }},
    { &hf_c2xx_edge,
        { "EdgeDataOutCommand", "c2xx.edge", FT_UINT8, BASE_DEC, VALS(SAMSUNG_IPC_DM_EdgeDataOutCommandType_string), 0x0,
            NULL, HFILL }},            
    { &hf_c2xx_data,
        { "Data", "c2xx.data", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
    { &hf_c2xx_stamp2,
        { "Stamp2", "c2xx.stamp2", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
/*           
    { &hf_c2xx_stamp2,
        { "Stamp2", "c2xx.stamp2", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL, NULL, 0x0,
            "Stamp2", HFILL }},
*/
    { &hf_c2xx_payload,
        { "Payload", "c2xx.payload", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
    { &hf_c2xx_todo,
        { "Todo", "c2xx.todo", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
    { &hf_c2xx_unk2,
        { "Unk2", "c2xx.unk2", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},

  };

  static gint *ett[] = {
        &ett_c2xx,
        &ett_c2xx_header,        
        &ett_c2xx_tag,
        &ett_c2xx_len,
        &ett_c2xx_sens,
        &ett_c2xx_ethdest,
        &ett_c2xx_ethsrc,
        &ett_c2xx_tag2,
        &ett_c2xx_hdlc,
        &ett_c2xx_start,
        &ett_c2xx_len1,
        &ett_c2xx_unk1,
        &ett_c2xx_len2,
        &ett_c2xx_trame,
        &ett_c2xx_end1,
        &ett_c2xx_stamp,
        &ett_c2xx_maincommand,
        &ett_c2xx_subcommand,
        &ett_c2xx_lte,
        &ett_c2xx_hspa,
        &ett_c2xx_edge,
        &ett_c2xx_data,
        &ett_c2xx_stamp2,
        &ett_c2xx_payload,
        &ett_c2xx_todo,
        &ett_c2xx_unk2
  };

  proto_c2xx = proto_register_protocol("C2xx Samsung Protocol", "C2XX", "c2xx");
  proto_register_field_array(proto_c2xx, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
  register_dissector("c2xx", dissect_c2xx, proto_c2xx);


}






static gboolean heur_dissect_c2xx(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
  const gchar c2xx_magic1[2] = {0x57, 0x43};
  const gchar c2xx_magic2[2] = {0x57, 0x44};
  
  
  if  ((tvb_memeql(tvb, 0, c2xx_magic1, sizeof(c2xx_magic1)) == 0) ||
       (tvb_memeql(tvb, 0, c2xx_magic2, sizeof(c2xx_magic2)) == 0)){
    
    dissect_c2xx(tvb, pinfo, tree);
    return (TRUE);
  }
  
  return (FALSE);
}




void
proto_reg_handoff_c2xx(void)
{
  //static dissector_handle_t c2xx_handle;

//  c2xx_handle = create_dissector_handle(dissect_c2xx, proto_c2xx);
  create_dissector_handle(dissect_c2xx, proto_c2xx);
  heur_dissector_add("usb.bulk", heur_dissect_c2xx, proto_c2xx);

  eth_withoutfcs_handle = find_dissector("eth_withoutfcs");
  ip_handle = find_dissector("ip");

}
