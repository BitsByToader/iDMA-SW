`include "register_interface/typedef.svh"
`include "idma/typedef.svh"
`include "axi/typedef.svh"
`include "axi/assign.svh"
`include "axi_stream/typedef.svh"
`include "axi_stream/assign.svh"

import axi_stream_test::*;
import axi_test::*;
import idma_desc64_reg_pkg::*;

module idma_desc64fe_axisbe_synth #(
    parameter int AddrWidth     = 64,
    parameter int DataWidth     = 64,
    parameter int StrbWidth     = 8,
    parameter int UserWidth     = 1,
    parameter int AxiIdWidth    = 3
) ();

    logic clk, rst_n, irq;

    typedef logic [DataWidth-1:0]   data_t;
    typedef logic [AddrWidth-1:0]   addr_t;
    typedef logic [StrbWidth-1:0]   strb_t;
    typedef logic [AxiIdWidth-1:0]  axis_id_t;
    typedef logic [AxiIdWidth-1:0]  axi_id_t;
    typedef logic [UserWidth-1:0]   user_t;

    `AXI_TYPEDEF_ALL(axi, /* addr */ addr_t, /* id */ axi_id_t, /* data */ data_t, /* strb */ strb_t, /* user */ user_t)
    `AXI_STREAM_TYPEDEF_S_CHAN_T(axis_t_chan_t, data_t, strb_t, strb_t, axis_id_t, axis_id_t, user_t)
    `AXI_STREAM_TYPEDEF_REQ_T(axis_req_t, axis_t_chan_t)
    `AXI_STREAM_TYPEDEF_RSP_T(axis_rsp_t)
    
    axi_req_t fe_reg_req;
    axi_resp_t fe_reg_rsp;
    axi_req_t desc_master_req, be_axi_req;
    axi_resp_t desc_master_rsp, be_axi_rsp;
    axis_req_t wr_stream_req, rd_stream_req;
    axis_rsp_t wr_stream_rsp, rd_stream_rsp;
    
    idma_desc64fe_axisbe_wrap #(
        .AddrWidth(AddrWidth),
        .AxiIdWidth(AxiIdWidth),
        .DataWidth(DataWidth),
        .StrbWidth(StrbWidth),
        .TFLenWidth(32),
        .UserWidth(UserWidth),
        .axi_req_t(axi_req_t),
        .axi_rsp_t(axi_resp_t),
        .axis_req_t(axis_req_t),
        .axis_rsp_t(axis_rsp_t),
        .axis_t_chan_t(axis_t_chan_t),
        .axi_ar_chan_t(axi_ar_chan_t),
        .axi_r_chan_t(axi_r_chan_t),
        .axi_aw_chan_t(axi_aw_chan_t),
        .axi_w_chan_t(axi_w_chan_t)
    ) dma (
        .clk_i(clk),
        .rst_ni(rst_n),
        
        .testmode_i(1'b0),
        .axi_ar_id_i(3'b111),
        .axi_aw_id_i(3'b111),
        
        .irq_o(irq),
        
        .slave_fe_req_i(fe_reg_req),
        .slave_fe_rsp_o(fe_reg_rsp),
        
        .master_fe_req_o(desc_master_req),
        .master_fe_rsp_i(desc_master_rsp),
        
        .streaming_wr_req_o(wr_stream_req),
        .streaming_wr_rsp_i(wr_stream_rsp),
        .streaming_rd_req_i(rd_stream_req),
        .streaming_rd_rsp_o(rd_stream_rsp),
        
        .master_be_axi_req_o(be_axi_req),
        .master_be_axi_rsp_i(be_axi_rsp)
    );
    
    `AXI_ASSIGN_SLAVE_TO_FLAT(regs, fe_reg_req, fe_reg_rsp)
    `AXI_ASSIGN_MASTER_TO_FLAT(desc, desc_master_req, desc_master_rsp)
    `AXI_ASSIGN_MASTER_TO_FLAT(be, be_axi_req, be_axi_rsp)
    
    `AXI_STREAM_ASSIGN_TO_FLAT(S_AXIS_0, wr_stream_req, wr_stream_rsp)
    `AXI_STREAM_ASSIGN_FROM_FLAT(M_AXIS_0, rd_stream_req, rd_stream_rsp)
    
endmodule