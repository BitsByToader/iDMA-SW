//Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
//Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
//--------------------------------------------------------------------------------
//Tool Version: Vivado v.2024.2 (lin64) Build 5239630 Fri Nov 08 22:34:34 MST 2024
//Date        : Sun Dec 14 12:55:59 2025
//Host        : david running 64-bit Ubuntu 22.04.5 LTS
//Command     : generate_target design_1_wrapper.bd
//Design      : design_1_wrapper
//Purpose     : IP block netlist
//--------------------------------------------------------------------------------
`timescale 1 ps / 1 ps

`include "register_interface/typedef.svh"
`include "idma/typedef.svh"
`include "axi/typedef.svh"
`include "axi/assign.svh"
`include "axi_stream/typedef.svh"
`include "axi_stream/assign.svh"

import idma_desc64_reg_pkg::*;

module design_1_wrapper();
  wire ACLK;
  wire [0:0]ARSTN;
  wire [0:0]A_IRQ;
  wire [63:0]M_AXIS_0_tdata;
  wire [2:0]M_AXIS_0_tdest;
  wire [2:0]M_AXIS_0_tid;
  wire [7:0]M_AXIS_0_tkeep;
  wire M_AXIS_0_tlast;
  wire M_AXIS_0_tready;
  wire [7:0]M_AXIS_0_tstrb;
  wire [0:0]M_AXIS_0_tuser;
  wire M_AXIS_0_tvalid;
  wire [63:0]S_AXIS_0_tdata;
  wire [2:0]S_AXIS_0_tdest;
  wire [2:0]S_AXIS_0_tid;
  wire [7:0]S_AXIS_0_tkeep;
  wire S_AXIS_0_tlast;
  wire S_AXIS_0_tready;
  wire [7:0]S_AXIS_0_tstrb;
  wire [0:0]S_AXIS_0_tuser;
  wire S_AXIS_0_tvalid;
  wire [48:0]m_axi_be_araddr;
  wire [1:0]m_axi_be_arburst;
  wire [3:0]m_axi_be_arcache;
  wire [5:0]m_axi_be_arid;
  wire [7:0]m_axi_be_arlen;
  wire m_axi_be_arlock;
  wire [2:0]m_axi_be_arprot;
  wire [3:0]m_axi_be_arqos;
  wire m_axi_be_arready;
  wire [2:0]m_axi_be_arsize;
  wire m_axi_be_aruser;
  wire m_axi_be_arvalid;
  wire [48:0]m_axi_be_awaddr;
  wire [1:0]m_axi_be_awburst;
  wire [3:0]m_axi_be_awcache;
  wire [5:0]m_axi_be_awid;
  wire [7:0]m_axi_be_awlen;
  wire m_axi_be_awlock;
  wire [2:0]m_axi_be_awprot;
  wire [3:0]m_axi_be_awqos;
  wire m_axi_be_awready;
  wire [2:0]m_axi_be_awsize;
  wire m_axi_be_awuser;
  wire m_axi_be_awvalid;
  wire [5:0]m_axi_be_bid;
  wire m_axi_be_bready;
  wire [1:0]m_axi_be_bresp;
  wire m_axi_be_bvalid;
  wire [63:0]m_axi_be_rdata;
  wire [5:0]m_axi_be_rid;
  wire m_axi_be_rlast;
  wire m_axi_be_rready;
  wire [1:0]m_axi_be_rresp;
  wire m_axi_be_rvalid;
  wire [63:0]m_axi_be_wdata;
  wire m_axi_be_wlast;
  wire m_axi_be_wready;
  wire [7:0]m_axi_be_wstrb;
  wire m_axi_be_wvalid;
  wire [48:0]m_axi_desc_araddr;
  wire [1:0]m_axi_desc_arburst;
  wire [3:0]m_axi_desc_arcache;
  wire [5:0]m_axi_desc_arid;
  wire [7:0]m_axi_desc_arlen;
  wire m_axi_desc_arlock;
  wire [2:0]m_axi_desc_arprot;
  wire [3:0]m_axi_desc_arqos;
  wire m_axi_desc_arready;
  wire [2:0]m_axi_desc_arsize;
  wire m_axi_desc_aruser;
  wire m_axi_desc_arvalid;
  wire [48:0]m_axi_desc_awaddr;
  wire [1:0]m_axi_desc_awburst;
  wire [3:0]m_axi_desc_awcache;
  wire [5:0]m_axi_desc_awid;
  wire [7:0]m_axi_desc_awlen;
  wire m_axi_desc_awlock;
  wire [2:0]m_axi_desc_awprot;
  wire [3:0]m_axi_desc_awqos;
  wire m_axi_desc_awready;
  wire [2:0]m_axi_desc_awsize;
  wire m_axi_desc_awuser;
  wire m_axi_desc_awvalid;
  wire [5:0]m_axi_desc_bid;
  wire m_axi_desc_bready;
  wire [1:0]m_axi_desc_bresp;
  wire m_axi_desc_bvalid;
  wire [63:0]m_axi_desc_rdata;
  wire [5:0]m_axi_desc_rid;
  wire m_axi_desc_rlast;
  wire m_axi_desc_rready;
  wire [1:0]m_axi_desc_rresp;
  wire m_axi_desc_rvalid;
  wire [63:0]m_axi_desc_wdata;
  wire m_axi_desc_wlast;
  wire m_axi_desc_wready;
  wire [7:0]m_axi_desc_wstrb;
  wire m_axi_desc_wvalid;
  wire [39:0]s_axi_regs_araddr;
  wire [1:0]s_axi_regs_arburst;
  wire [3:0]s_axi_regs_arcache;
  wire [15:0]s_axi_regs_arid;
  wire [7:0]s_axi_regs_arlen;
  wire s_axi_regs_arlock;
  wire [2:0]s_axi_regs_arprot;
  wire [3:0]s_axi_regs_arqos;
  wire s_axi_regs_arready;
  wire [2:0]s_axi_regs_arsize;
  wire [15:0]s_axi_regs_aruser;
  wire s_axi_regs_arvalid;
  wire [39:0]s_axi_regs_awaddr;
  wire [1:0]s_axi_regs_awburst;
  wire [3:0]s_axi_regs_awcache;
  wire [15:0]s_axi_regs_awid;
  wire [7:0]s_axi_regs_awlen;
  wire s_axi_regs_awlock;
  wire [2:0]s_axi_regs_awprot;
  wire [3:0]s_axi_regs_awqos;
  wire s_axi_regs_awready;
  wire [2:0]s_axi_regs_awsize;
  wire [15:0]s_axi_regs_awuser;
  wire s_axi_regs_awvalid;
  wire [15:0]s_axi_regs_bid;
  wire s_axi_regs_bready;
  wire [1:0]s_axi_regs_bresp;
  wire s_axi_regs_bvalid;
  wire [63:0]s_axi_regs_rdata;
  wire [15:0]s_axi_regs_rid;
  wire s_axi_regs_rlast;
  wire s_axi_regs_rready;
  wire [1:0]s_axi_regs_rresp;
  wire s_axi_regs_rvalid;
  wire [63:0]s_axi_regs_wdata;
  wire s_axi_regs_wlast;
  wire s_axi_regs_wready;
  wire [7:0]s_axi_regs_wstrb;
  wire s_axi_regs_wvalid;

  design_1 design_1_i
       (.ACLK(ACLK),
        .ARSTN(ARSTN),
        .A_IRQ(A_IRQ),
        .M_AXIS_0_tdata(M_AXIS_0_tdata),
        .M_AXIS_0_tdest(M_AXIS_0_tdest),
        .M_AXIS_0_tid(M_AXIS_0_tid),
        .M_AXIS_0_tkeep(M_AXIS_0_tkeep),
        .M_AXIS_0_tlast(M_AXIS_0_tlast),
        .M_AXIS_0_tready(M_AXIS_0_tready),
        .M_AXIS_0_tstrb(M_AXIS_0_tstrb),
        .M_AXIS_0_tuser(M_AXIS_0_tuser),
        .M_AXIS_0_tvalid(M_AXIS_0_tvalid),
        .S_AXIS_0_tdata(S_AXIS_0_tdata),
        .S_AXIS_0_tdest(S_AXIS_0_tdest),
        .S_AXIS_0_tid(S_AXIS_0_tid),
        .S_AXIS_0_tkeep(S_AXIS_0_tkeep),
        .S_AXIS_0_tlast(S_AXIS_0_tlast),
        .S_AXIS_0_tready(S_AXIS_0_tready),
        .S_AXIS_0_tstrb(S_AXIS_0_tstrb),
        .S_AXIS_0_tuser(S_AXIS_0_tuser),
        .S_AXIS_0_tvalid(S_AXIS_0_tvalid),
        .m_axi_be_araddr(m_axi_be_araddr),
        .m_axi_be_arburst(m_axi_be_arburst),
        .m_axi_be_arcache(m_axi_be_arcache),
        .m_axi_be_arid(m_axi_be_arid),
        .m_axi_be_arlen(m_axi_be_arlen),
        .m_axi_be_arlock(m_axi_be_arlock),
        .m_axi_be_arprot(m_axi_be_arprot),
        .m_axi_be_arqos(m_axi_be_arqos),
        .m_axi_be_arready(m_axi_be_arready),
        .m_axi_be_arsize(m_axi_be_arsize),
        .m_axi_be_aruser(m_axi_be_aruser),
        .m_axi_be_arvalid(m_axi_be_arvalid),
        .m_axi_be_awaddr(m_axi_be_awaddr),
        .m_axi_be_awburst(m_axi_be_awburst),
        .m_axi_be_awcache(m_axi_be_awcache),
        .m_axi_be_awid(m_axi_be_awid),
        .m_axi_be_awlen(m_axi_be_awlen),
        .m_axi_be_awlock(m_axi_be_awlock),
        .m_axi_be_awprot(m_axi_be_awprot),
        .m_axi_be_awqos(m_axi_be_awqos),
        .m_axi_be_awready(m_axi_be_awready),
        .m_axi_be_awsize(m_axi_be_awsize),
        .m_axi_be_awuser(m_axi_be_awuser),
        .m_axi_be_awvalid(m_axi_be_awvalid),
        .m_axi_be_bid(m_axi_be_bid),
        .m_axi_be_bready(m_axi_be_bready),
        .m_axi_be_bresp(m_axi_be_bresp),
        .m_axi_be_bvalid(m_axi_be_bvalid),
        .m_axi_be_rdata(m_axi_be_rdata),
        .m_axi_be_rid(m_axi_be_rid),
        .m_axi_be_rlast(m_axi_be_rlast),
        .m_axi_be_rready(m_axi_be_rready),
        .m_axi_be_rresp(m_axi_be_rresp),
        .m_axi_be_rvalid(m_axi_be_rvalid),
        .m_axi_be_wdata(m_axi_be_wdata),
        .m_axi_be_wlast(m_axi_be_wlast),
        .m_axi_be_wready(m_axi_be_wready),
        .m_axi_be_wstrb(m_axi_be_wstrb),
        .m_axi_be_wvalid(m_axi_be_wvalid),
        .m_axi_desc_araddr(m_axi_desc_araddr),
        .m_axi_desc_arburst(m_axi_desc_arburst),
        .m_axi_desc_arcache(m_axi_desc_arcache),
        .m_axi_desc_arid(m_axi_desc_arid),
        .m_axi_desc_arlen(m_axi_desc_arlen),
        .m_axi_desc_arlock(m_axi_desc_arlock),
        .m_axi_desc_arprot(m_axi_desc_arprot),
        .m_axi_desc_arqos(m_axi_desc_arqos),
        .m_axi_desc_arready(m_axi_desc_arready),
        .m_axi_desc_arsize(m_axi_desc_arsize),
        .m_axi_desc_aruser(m_axi_desc_aruser),
        .m_axi_desc_arvalid(m_axi_desc_arvalid),
        .m_axi_desc_awaddr(m_axi_desc_awaddr),
        .m_axi_desc_awburst(m_axi_desc_awburst),
        .m_axi_desc_awcache(m_axi_desc_awcache),
        .m_axi_desc_awid(m_axi_desc_awid),
        .m_axi_desc_awlen(m_axi_desc_awlen),
        .m_axi_desc_awlock(m_axi_desc_awlock),
        .m_axi_desc_awprot(m_axi_desc_awprot),
        .m_axi_desc_awqos(m_axi_desc_awqos),
        .m_axi_desc_awready(m_axi_desc_awready),
        .m_axi_desc_awsize(m_axi_desc_awsize),
        .m_axi_desc_awuser(m_axi_desc_awuser),
        .m_axi_desc_awvalid(m_axi_desc_awvalid),
        .m_axi_desc_bid(m_axi_desc_bid),
        .m_axi_desc_bready(m_axi_desc_bready),
        .m_axi_desc_bresp(m_axi_desc_bresp),
        .m_axi_desc_bvalid(m_axi_desc_bvalid),
        .m_axi_desc_rdata(m_axi_desc_rdata),
        .m_axi_desc_rid(m_axi_desc_rid),
        .m_axi_desc_rlast(m_axi_desc_rlast),
        .m_axi_desc_rready(m_axi_desc_rready),
        .m_axi_desc_rresp(m_axi_desc_rresp),
        .m_axi_desc_rvalid(m_axi_desc_rvalid),
        .m_axi_desc_wdata(m_axi_desc_wdata),
        .m_axi_desc_wlast(m_axi_desc_wlast),
        .m_axi_desc_wready(m_axi_desc_wready),
        .m_axi_desc_wstrb(m_axi_desc_wstrb),
        .m_axi_desc_wvalid(m_axi_desc_wvalid),
        .s_axi_regs_araddr(s_axi_regs_araddr),
        .s_axi_regs_arburst(s_axi_regs_arburst),
        .s_axi_regs_arcache(s_axi_regs_arcache),
        .s_axi_regs_arid(s_axi_regs_arid),
        .s_axi_regs_arlen(s_axi_regs_arlen),
        .s_axi_regs_arlock(s_axi_regs_arlock),
        .s_axi_regs_arprot(s_axi_regs_arprot),
        .s_axi_regs_arqos(s_axi_regs_arqos),
        .s_axi_regs_arready(s_axi_regs_arready),
        .s_axi_regs_arsize(s_axi_regs_arsize),
        .s_axi_regs_aruser(s_axi_regs_aruser),
        .s_axi_regs_arvalid(s_axi_regs_arvalid),
        .s_axi_regs_awaddr(s_axi_regs_awaddr),
        .s_axi_regs_awburst(s_axi_regs_awburst),
        .s_axi_regs_awcache(s_axi_regs_awcache),
        .s_axi_regs_awid(s_axi_regs_awid),
        .s_axi_regs_awlen(s_axi_regs_awlen),
        .s_axi_regs_awlock(s_axi_regs_awlock),
        .s_axi_regs_awprot(s_axi_regs_awprot),
        .s_axi_regs_awqos(s_axi_regs_awqos),
        .s_axi_regs_awready(s_axi_regs_awready),
        .s_axi_regs_awsize(s_axi_regs_awsize),
        .s_axi_regs_awuser(s_axi_regs_awuser),
        .s_axi_regs_awvalid(s_axi_regs_awvalid),
        .s_axi_regs_bid(s_axi_regs_bid),
        .s_axi_regs_bready(s_axi_regs_bready),
        .s_axi_regs_bresp(s_axi_regs_bresp),
        .s_axi_regs_bvalid(s_axi_regs_bvalid),
        .s_axi_regs_rdata(s_axi_regs_rdata),
        .s_axi_regs_rid(s_axi_regs_rid),
        .s_axi_regs_rlast(s_axi_regs_rlast),
        .s_axi_regs_rready(s_axi_regs_rready),
        .s_axi_regs_rresp(s_axi_regs_rresp),
        .s_axi_regs_rvalid(s_axi_regs_rvalid),
        .s_axi_regs_wdata(s_axi_regs_wdata),
        .s_axi_regs_wlast(s_axi_regs_wlast),
        .s_axi_regs_wready(s_axi_regs_wready),
        .s_axi_regs_wstrb(s_axi_regs_wstrb),
        .s_axi_regs_wvalid(s_axi_regs_wvalid));
        
    // 49bits is the largest address used in the SoC. However, iDMA calculates descriptor size/length using this width. User must take care not to send jobs which overflow the true address space.
    // I will keep 64 for simplicity (CVA will use true 64bit bus) & (Simulation is also true 64bit)...
    parameter int AddrWidth     = 64;
    parameter int DataWidth     = 64;
    parameter int StrbWidth     = 8;
    parameter int UserWidth     = 1;
    parameter int AxiIdWidth    = 16;   
    
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
        .NSpeculation(0),
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
        .clk_i(ACLK),
        .rst_ni(ARSTN),
        
        .testmode_i(1'b0),
        .axi_ar_id_i('b0),
        .axi_aw_id_i('b0),
        
        .irq_o(A_IRQ),
        
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
    
    logic m_axi_desc_buser, m_axi_desc_ruser;
    logic m_axi_be_buser, m_axi_be_ruser;
    logic s_axi_regs_awregion, s_axi_regs_wuser, s_axi_regs_arregion;
    
    `AXI_ASSIGN_SLAVE_TO_FLAT(regs, fe_reg_req, fe_reg_rsp)
    `AXI_ASSIGN_MASTER_TO_FLAT(desc, desc_master_req, desc_master_rsp)
    `AXI_ASSIGN_MASTER_TO_FLAT(be, be_axi_req, be_axi_rsp)
    
    `AXI_STREAM_ASSIGN_TO_FLAT(M_AXIS_0, wr_stream_req, wr_stream_rsp)
    `AXI_STREAM_ASSIGN_FROM_FLAT(rd_stream_req, rd_stream_rsp, S_AXIS_0)
        
        
endmodule
