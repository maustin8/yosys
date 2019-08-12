/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2012  Clifford Wolf <clifford@clifford.at>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "kernel/yosys.h"
#include "kernel/sigtools.h"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct XilinxBufgMapPass : public Pass {
	XilinxBufgMapPass() : Pass("xilinx_bufgmap", "insert global buffers on clock networks") { }
	void help() YS_OVERRIDE
	{
		log("\n");
		log("    xilinx_bufgmap [options] [selection]\n");
		log("\n");
		log("Inserts BUFG cells between nets driving clock inputs and their\n");
		log("drivers.\n");
		log("\n");
		log("    -pad\n");
		log("        Inserts IBUFG cells on module inputs that would become\n");
		log("        BUFG inputs.\n");
		log("\n");
	}
	void execute(std::vector<std::string> args, RTLIL::Design *design) YS_OVERRIDE
	{
		log_header(design, "Executing XILINX_BUFGMAP pass (inserting BUFG cells).\n");

		bool pad = false;
		pool<pair<IdString, IdString>> clock_ports;
		pool<pair<IdString, IdString>> buf_ports;

		size_t argidx;
		for (argidx = 1; argidx < args.size(); argidx++)
		{
			std::string arg = args[argidx];
			if (arg == "-pad") {
				pad = true;
				continue;
			}
			break;
		}
		extra_args(args, argidx, design);

		clock_ports.insert(make_pair(IdString("\\FDRE"), IdString("\\C")));
		clock_ports.insert(make_pair(IdString("\\FDSE"), IdString("\\C")));
		clock_ports.insert(make_pair(IdString("\\FDPE"), IdString("\\C")));
		clock_ports.insert(make_pair(IdString("\\FDCE"), IdString("\\C")));
		clock_ports.insert(make_pair(IdString("\\FDRE_1"), IdString("\\C")));
		clock_ports.insert(make_pair(IdString("\\FDSE_1"), IdString("\\C")));
		clock_ports.insert(make_pair(IdString("\\FDPE_1"), IdString("\\C")));
		clock_ports.insert(make_pair(IdString("\\FDCE_1"), IdString("\\C")));
		clock_ports.insert(make_pair(IdString("\\RAM32X1D"), IdString("\\WCLK")));
		clock_ports.insert(make_pair(IdString("\\RAM64X1D"), IdString("\\WCLK")));
		clock_ports.insert(make_pair(IdString("\\RAM128X1D"), IdString("\\WCLK")));
		clock_ports.insert(make_pair(IdString("\\RAM32X1S"), IdString("\\WCLK")));
		clock_ports.insert(make_pair(IdString("\\RAM64X1S"), IdString("\\WCLK")));
		clock_ports.insert(make_pair(IdString("\\RAM128X1S"), IdString("\\WCLK")));
		clock_ports.insert(make_pair(IdString("\\RAM256X1S"), IdString("\\WCLK")));
		clock_ports.insert(make_pair(IdString("\\RAM32X1S_1"), IdString("\\WCLK")));
		clock_ports.insert(make_pair(IdString("\\RAM64X1S_1"), IdString("\\WCLK")));
		clock_ports.insert(make_pair(IdString("\\RAM128X1S_1"), IdString("\\WCLK")));
		clock_ports.insert(make_pair(IdString("\\RAM256X1S_1"), IdString("\\WCLK")));
		clock_ports.insert(make_pair(IdString("\\RAM32X2S"), IdString("\\WCLK")));
		clock_ports.insert(make_pair(IdString("\\RAM64X2S"), IdString("\\WCLK")));
		clock_ports.insert(make_pair(IdString("\\RAM32M"), IdString("\\WCLK")));
		clock_ports.insert(make_pair(IdString("\\SRL16E"), IdString("\\CLK")));
		clock_ports.insert(make_pair(IdString("\\SRLC32E"), IdString("\\CLK")));
		clock_ports.insert(make_pair(IdString("\\RAMB18E1"), IdString("\\CLKARDCLK")));
		clock_ports.insert(make_pair(IdString("\\RAMB18E1"), IdString("\\CLKBWRCLK")));
		clock_ports.insert(make_pair(IdString("\\RAMB36E1"), IdString("\\CLKARDCLK")));
		clock_ports.insert(make_pair(IdString("\\RAMB36E1"), IdString("\\CLKBWRCLK")));
		clock_ports.insert(make_pair(IdString("\\FIFO18E1"), IdString("\\RDCLK")));
		clock_ports.insert(make_pair(IdString("\\FIFO18E1"), IdString("\\WRCLK")));
		clock_ports.insert(make_pair(IdString("\\FIFO36E1"), IdString("\\RDCLK")));
		clock_ports.insert(make_pair(IdString("\\FIFO36E1"), IdString("\\WRCLK")));
		clock_ports.insert(make_pair(IdString("\\RAMB8BWER"), IdString("\\CLKAWRCLK")));
		clock_ports.insert(make_pair(IdString("\\RAMB8BWER"), IdString("\\CLKBRDCLK")));
		clock_ports.insert(make_pair(IdString("\\RAMB16BWER"), IdString("\\CLKA")));
		clock_ports.insert(make_pair(IdString("\\RAMB16BWER"), IdString("\\CLKB")));
		clock_ports.insert(make_pair(IdString("\\DSP48A1"), IdString("\\CLK")));
		clock_ports.insert(make_pair(IdString("\\DSP48E1"), IdString("\\CLK")));

		buf_ports.insert(make_pair(IdString("\\BUFG"), IdString("\\O")));
		buf_ports.insert(make_pair(IdString("\\BUFGCE"), IdString("\\O")));
		buf_ports.insert(make_pair(IdString("\\BUFGCE_1"), IdString("\\O")));
		buf_ports.insert(make_pair(IdString("\\BUFGMUX"), IdString("\\O")));
		buf_ports.insert(make_pair(IdString("\\BUFGMUX_CTRL"), IdString("\\O")));
		buf_ports.insert(make_pair(IdString("\\BUFGCTRL"), IdString("\\O")));
		buf_ports.insert(make_pair(IdString("\\BUFH"), IdString("\\O")));
		buf_ports.insert(make_pair(IdString("\\BUFHCE"), IdString("\\O")));
		buf_ports.insert(make_pair(IdString("\\BUFR"), IdString("\\O")));
		buf_ports.insert(make_pair(IdString("\\BUFMR"), IdString("\\O")));
		buf_ports.insert(make_pair(IdString("\\BUFMRCE"), IdString("\\O")));
		buf_ports.insert(make_pair(IdString("\\BUFIO"), IdString("\\O")));

		for (auto module : design->selected_modules())
		{
			dict<IdString, pool<int>> clock_wires;
			pool<SigBit> clock_wire_bits;
			SigMap sigmap(module);
			dict<SigBit, pair<Cell *, Wire *>> buffered_bits;

			// First, collect nets that could use a clock buffer.
			for (auto cell : module->cells())
			for (auto port : cell->connections())
				if (clock_ports.count(make_pair(cell->type, port.first)))
					for (auto bit : sigmap(port.second))
						clock_wire_bits.insert(bit);

			// Second, discard ones that already have a clock buffer.
			for (auto cell : module->cells())
			for (auto port : cell->connections())
				if (buf_ports.count(make_pair(cell->type, port.first)))
					for (auto bit : sigmap(port.second))
						clock_wire_bits.erase(bit);

			std::vector<pair<Wire *, pool<int>>> ibufg_queue;
			for (auto wire : module->selected_wires())
			{
				if (wire->get_bool_attribute("\\skip_bufgmap"))
					continue;

				pool<int> ibufg_bits;

				for (int i = 0; i < GetSize(wire); i++)
				{
					SigBit wire_bit(wire, i);
					SigBit mapped_wire_bit = sigmap(wire_bit);
					// Only map the canonical wire of a given SigBit.
					if (wire_bit != mapped_wire_bit)
						continue;
					if (!clock_wire_bits.count(mapped_wire_bit))
						continue;

					log("Inserting BUFG on %s.%s[%d].\n", log_id(module), log_id(wire), i);
					RTLIL::Cell *cell = module->addCell(NEW_ID, "\\BUFG");
					Wire *owire = module->addWire(NEW_ID);
					cell->setPort("\\O", owire);
					cell->setPort("\\I", mapped_wire_bit);
					buffered_bits[mapped_wire_bit] = make_pair(cell, owire);

					if (wire->port_input && !wire->port_output && pad) {
						ibufg_bits.insert(i);
					}
				}
				if (!ibufg_bits.empty()) {
					ibufg_queue.push_back(make_pair(wire, ibufg_bits));
				}
			}

			for (auto cell : module->cells())
			for (auto port : cell->connections()) {
				if (!cell->input(port.first) || cell->output(port.first))
					continue;
				SigSpec sig = port.second;
				bool newsig = false;
				for (auto &bit : sig) {
					const auto it = buffered_bits.find(sigmap(bit));
					if (it == buffered_bits.end())
						continue;
					// Avoid substituting BUFG's own input pin.
					if (cell == it->second.first)
						continue;
					bit = it->second.second;
					newsig = true;
				}
				if (newsig)
					cell->setPort(port.first, sig);
			}

			// This has to be done last, to avoid upsetting sigmap before the port reconnections.
			for (auto &it : ibufg_queue) {
				Wire *wire = it.first;
				pool<int> &ibufg_bits = it.second;
				Wire *new_wire = module->addWire(NEW_ID, wire);
				module->swap_names(new_wire, wire);
				wire->attributes.clear();
				for (int i = 0; i < wire->width; i++) {
					if (ibufg_bits.count(i)) {
						log("Inserting IBUFG on %s.%s[%d].\n", log_id(module), log_id(new_wire), i);
						RTLIL::Cell *cell = module->addCell(NEW_ID, "\\IBUFG");
						cell->setPort("\\O", SigSpec(wire, i));
						cell->setPort("\\I", SigSpec(new_wire, i));
					} else {
						module->connect(SigSpec(wire, i), SigSpec(new_wire, i));
					}
				}
				wire->port_id = 0;
				wire->port_input = false;
				wire->port_output = false;
			}

			module->fixup_ports();
		}
	}
} XilinxBufgMapPass;

PRIVATE_NAMESPACE_END
