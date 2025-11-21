`include "register_interface/typedef.svh"
`include "register_interface/assign.svh"
`include "idma/typedef.svh"
`include "axi/typedef.svh"
`include "axi/assign.svh"
`include "axi_stream/typedef.svh"

module tb_idma_desc64fe_axisbe();
    logic clk, rst;
    
    initial begin
        clk = 1;
        forever #5 clk = ~clk;
    end
    
    initial begin
        rst = 1;
        #15
        rst = 0;
    end
    
    initial begin
        $dumpfile("dump.fst");
        $dumpvars();
        
        #1000 $finish();
    end
    
    typedef logic [63:0] data_t;
    typedef logic [63:0] addr_t;
    typedef logic [ 7:0] strb_t;
    typedef logic [ 2:0] axis_id_t;
    typedef logic [ 2:0] axi_id_t;
    typedef logic [23:0] tf_len_t;
    typedef logic [ 0:0] user_t;
    
    `REG_BUS_TYPEDEF_ALL(reg, /* addr */ addr_t, /* data */ data_t, /* strobe */ strb_t)
    `AXI_TYPEDEF_ALL(axi, /* addr */ addr_t, /* id */ axi_id_t, /* data */ data_t, /* strb */ strb_t, /* user */ user_t)
    
    `AXI_STREAM_TYPEDEF_S_CHAN_T(axis_t_chan_t, data_t, strb_t, strb_t, axis_id_t, axis_id_t, user_t)
    `AXI_STREAM_TYPEDEF_REQ_T(axis_req_t, axis_t_chan_t)
    `AXI_STREAM_TYPEDEF_RSP_T(axis_rsp_t)
    
    reg_req_t bus_req;
    reg_rsp_t bus_rsp;
    axi_req_t master_req;
    axi_resp_t master_rsp;
    axis_req_t wr_stream_req, rd_stream_req;
    axis_rsp_t wr_stream_rsp, rd_stream_rsp;
    
    idma_desc64fe_axisbe_wrap #(
        .AddrWidth(64),
        .AxiIdWidth(3),
        .DataWidth(64),
        .MaskInvalidData(0),
        .StrbWidth(8),
        .TFLenWidth(24),
        .UserWidth(1),
        .axi_req_t(axi_req_t),
        .axi_rsp_t(axi_resp_t),
        .axis_req_t(axis_req_t),
        .axis_rsp_t(axis_rsp_t),
        .axis_t_chan_t(axis_t_chan_t),
        .reg_req_t(reg_req_t),
        .reg_rsp_t(reg_rsp_t),
        .axi_ar_chan_t(axi_ar_chan_t),
        .axi_r_chan_t(axi_r_chan_t),
        .axi_aw_chan_t(axi_aw_chan_t),
        .axi_w_chan_t(axi_w_chan_t)
    ) dma (
        .clk_i(clk),
        .rst_ni(rst),
        .testmode_i(1'b0),
        .axi_ar_id_i(3'b111),
        .axi_aw_id_i(3'b111),
        .irq_o(),
        .slave_req_i(bus_req),
        .slave_rsp_o(bus_rsp),
        .master_fe_req_o(master_req),
        .master_fe_rsp_i(master_rsp),
        .streaming_wr_req_o(wr_stream_req),
        .streaming_wr_rsp_i(wr_stream_rsp),
        .streaming_rd_req_i(rd_stream_req),
        .streaming_rd_rsp_o(rd_stream_rsp)
    );
    
endmodule