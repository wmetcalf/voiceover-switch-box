#!/usr/bin/env python3
"""
VoiceOver + TD Snap Adaptive Device — carrier PCB v1 (SKiDL)

Generates:
  voiceover_pcb.net   KiCad netlist  (PCB Editor: File > Import > Netlist)
  voiceover_pcb_bom.csv

The Pico 2 W plugs into two 1x20 sockets (socketed so it can be swapped).
Place the sockets 17.78 mm (0.7") apart, centre to centre:
  J1 = Pico pins 1-20  (left edge,  pin 1 next to the USB end)
  J2 = Pico pins 21-40 (right edge; rotate 180° so socket pin 1 = Pico pin 21,
       which is the end AWAY from USB)

Everything off-board (buttons, encoder, power light, battery, power switch) plugs
in through JST-XH (2.5 mm) or JST-PH (battery) connectors.

Firmware v10 pin map is the source of truth; keep the two in sync.
"""
import csv
import sys
from skidl import Part, Pin, Net, SKIDL, TEMPLATE, KICAD9, set_default_tool, generate_netlist, ERC
import builtins
default_circuit = builtins.default_circuit

set_default_tool(KICAD9)

# ── footprints (all verified to exist in the stock KiCad footprint library) ──
FP_SOCKET20 = "Connector_PinSocket_2.54mm:PinSocket_1x20_P2.54mm_Vertical"
FP_XH2      = "Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical"
FP_XH4      = "Connector_JST:JST_XH_B4B-XH-A_1x04_P2.50mm_Vertical"
FP_PH2      = "Connector_JST:JST_PH_B2B-PH-K_1x02_P2.00mm_Vertical"
FP_HDR2     = "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical"
FP_R        = "Resistor_SMD:R_0805_2012Metric"
FP_C        = "Capacitor_SMD:C_0805_2012Metric"
FP_LED      = "LED_SMD:LED_0805_2012Metric"
FP_SOD123   = "Diode_SMD:D_SOD-123"
FP_SOD323   = "Diode_SMD:D_SOD-323"
FP_SOT23    = "Package_TO_SOT_SMD:SOT-23"
FP_SOT23_5  = "Package_TO_SOT_SMD:SOT-23-5"
FP_BUZZER   = "Buzzer_Beeper:Buzzer_12x9.5RM7.6"
FP_JP3      = "Jumper:SolderJumper-3_P1.3mm_Open_RoundedPad1.0x1.5mm"
FP_HOLE     = "MountingHole:MountingHole_3.2mm_M3"


def template(name, ref, fp, pins, desc=""):
    """pins: list of (number, name)."""
    return Part(tool=SKIDL, name=name, ref_prefix=ref, footprint=fp, dest=TEMPLATE,
                description=desc,
                pins=[Pin(num=str(n), name=nm, func=Pin.types.PASSIVE) for n, nm in pins])


SOCKET20 = template("Pico_Socket_1x20", "J", FP_SOCKET20, [(i, f"P{i}") for i in range(1, 21)])
XH2   = template("Conn_XH_2", "J", FP_XH2, [(1, "SIG"), (2, "GND")])
XH4   = template("Conn_XH_4", "J", FP_XH4, [(1, "VCC"), (2, "GND"), (3, "A"), (4, "B")])
XH2SW = template("Conn_XH_2_Switch", "J", FP_XH2, [(1, "T1"), (2, "T2")])
PH2   = template("Conn_PH_2", "J", FP_PH2, [(1, "+"), (2, "-")])
HDR2  = template("Header_2", "J", FP_HDR2, [(1, "1"), (2, "2")])
R     = template("R", "R", FP_R, [(1, "1"), (2, "2")])
C     = template("C", "C", FP_C, [(1, "1"), (2, "2")])
LED   = template("LED", "D", FP_LED, [(1, "K"), (2, "A")])
DSCH  = template("D_Schottky", "D", FP_SOD123, [(1, "K"), (2, "A")])
DSIG  = template("D_Small", "D", FP_SOD323, [(1, "K"), (2, "A")])
NPN   = template("Q_NPN_BEC", "Q", FP_SOT23, [(1, "B"), (2, "E"), (3, "C")])
MCP   = template("MCP73831", "U", FP_SOT23_5,
                 [(1, "STAT"), (2, "VSS"), (3, "VBAT"), (4, "VDD"), (5, "PROG")])
BUZ   = template("Buzzer", "BZ", FP_BUZZER, [(1, "+"), (2, "-")])
JP3   = template("SolderJumper_3", "JP", FP_JP3, [(1, "A"), (2, "C"), (3, "B")])
HOLE  = template("MountingHole", "H", FP_HOLE, [])

# ── power nets ──
gnd   = Net("GND")
v3v3  = Net("+3V3")
vsys  = Net("VSYS")
vbus  = Net("VBUS")
bat   = Net("BAT+")       # battery terminal (charger side)
batsw = Net("BAT_SW")     # after the power switch
for n in (gnd, v3v3, vsys, vbus):
    n.drive = 3  # mark as power so ERC doesn't flag them as undriven

# ── signal nets (names match firmware v10) ──
nets = {name: Net(name) for name in [
    "ENC_A", "ENC_B", "SEL", "RED", "GRN", "BLU", "YLW", "CHO", "BACK",
    "PIEZO_DRV", "BATT_SENSE", "RUN"]}

# ── Pico 2 W sockets ──
picoL = SOCKET20(value="Pico 2 W pins 1-20")
picoR = SOCKET20(value="Pico 2 W pins 21-40")

def pico(pin):
    """Return the socket pin for physical Pico pin 1..40."""
    return picoL[str(pin)] if pin <= 20 else picoR[str(pin - 20)]

for p in (3, 8, 13, 18, 23, 28, 33, 38):           # all Pico GND pins (33 = AGND)
    gnd += pico(p)
nets["ENC_A"] += pico(14)    # GP10
nets["ENC_B"] += pico(15)    # GP11
nets["SEL"] += pico(20)      # GP15
nets["RED"] += pico(21)      # GP16
nets["GRN"] += pico(22)      # GP17
nets["BLU"] += pico(24)      # GP18
nets["YLW"] += pico(25)      # GP19
nets["CHO"] += pico(26)      # GP20
nets["BACK"] += pico(27)     # GP21
nets["PIEZO_DRV"] += pico(29)  # GP22
nets["RUN"] += pico(30)
nets["BATT_SENSE"] += pico(31)  # GP26 / ADC0
v3v3 += pico(36)
vsys += pico(39)
vbus += pico(40)
unused = [1, 2, 4, 5, 6, 7, 9, 10, 11, 12, 16, 17, 19, 32, 34, 35, 37]   # GP0-9,12,13,14,27,28, ADC_VREF, 3V3_EN
for p in unused:
    pico(p).do_erc = False

# ── panel buttons: one 2-pin XH each (pin 1 = signal, pin 2 = GND) ──
for name, label in [("SEL", "Select 100mm"), ("RED", "Red"), ("GRN", "Green"),
                    ("BLU", "Blue"), ("YLW", "Yellow"), ("CHO", "Chooser"), ("BACK", "Back/Escape")]:
    j = XH2(value=f"BTN {label}")
    j["SIG"] += nets[name]
    j["GND"] += gnd

# ── encoder: XH4 (VCC, GND, A, B) with selectable supply and optional 5V divider ──
enc_vcc = Net("ENC_VCC")
jenc = XH4(value="Encoder")
jenc["VCC"] += enc_vcc
jenc["GND"] += gnd
jp_enc = JP3(value="ENC VCC: A=3V3 (default) / B=VSYS")
jp_enc["A"] += v3v3
jp_enc["C"] += enc_vcc
jp_enc["B"] += vsys
c_enc = C(value="100nF")
c_enc[1] += enc_vcc; c_enc[2] += gnd
for ch in ("A", "B"):
    raw = Net(f"ENC_{ch}_RAW")
    jenc[ch] += raw
    rs = R(value="1k")                        # series protection
    rs[1] += raw; rs[2] += nets[f"ENC_{ch}"]
    rb = R(value="2k DNP (fit only if ENC VCC=VSYS)")   # 1k/2k divider brings 5V logic to 3.3V
    rb[1] += nets[f"ENC_{ch}"]; rb[2] += gnd

# ── piezo / magnetic buzzer, low-side NPN driver ──
bz_neg = Net("BZ_NEG")
q_base = Net("Q_BASE")
bz = BUZ(value="Magnetic buzzer 3V, ~2.7kHz (or passive piezo)")
bz["+"] += v3v3
bz["-"] += bz_neg
q = NPN(value="MMBT3904")
q["C"] += bz_neg; q["E"] += gnd; q["B"] += q_base
rb = R(value="1k"); rb[1] += nets["PIEZO_DRV"]; rb[2] += q_base
rpd = R(value="100k"); rpd[1] += q_base; rpd[2] += gnd
dfly = DSIG(value="1N4148W"); dfly["K"] += v3v3; dfly["A"] += bz_neg
c_bulk = C(value="10uF"); c_bulk[1] += v3v3; c_bulk[2] += gnd

# ── LiPo battery, charger, power switch, OR-ing diode into VSYS ──
jbat = PH2(value="LiPo 1S (JST-PH) — check polarity!")
jbat["+"] += bat; jbat["-"] += gnd
jsw = XH2SW(value="Power switch (panel SPST)")
jsw["T1"] += bat; jsw["T2"] += batsw
d_or = DSCH(value="B5819W")
d_or["A"] += batsw; d_or["K"] += vsys          # Pico already has its own VBUS->VSYS Schottky
chg = MCP(value="MCP73831T-2ACI/OT")
stat = Net("CHG_STAT"); prog = Net("CHG_PROG")
chg["VDD"] += vbus; chg["VSS"] += gnd; chg["VBAT"] += bat
chg["PROG"] += prog; chg["STAT"] += stat
r_prog = R(value="4.7k (~210mA charge)"); r_prog[1] += prog; r_prog[2] += gnd
c_in = C(value="4.7uF"); c_in[1] += vbus; c_in[2] += gnd
c_bat = C(value="4.7uF"); c_bat[1] += bat; c_bat[2] += gnd
led = LED(value="Red - charging"); led["A"] += Net("LED_CHG_A"); led["K"] += stat
r_led = R(value="1k"); r_led[1] += vbus; r_led[2] += led["A"]
# battery sense on GP26: 100k/100k from BAT_SW (no drain when switched off)
r_top = R(value="100k"); r_top[1] += batsw; r_top[2] += nets["BATT_SENSE"]
r_bot = R(value="100k"); r_bot[1] += nets["BATT_SENSE"]; r_bot[2] += gnd
c_sns = C(value="100nF"); c_sns[1] += nets["BATT_SENSE"]; c_sns[2] += gnd

# ── reset header (short RUN to GND to reset) ──
jrst = HDR2(value="RESET")
jrst[1] += nets["RUN"]; jrst[2] += gnd

# ── power light: the Red button's LED (5 V, built-in resistor) straight from VBUS ──
jled = XH2(value="Red button LED (power light): 1 = LED + (VBUS), 2 = LED - (GND)")
jled["SIG"] += vbus; jled["GND"] += gnd

# ── mounting holes ──
for _ in range(4):
    HOLE(value="M3")


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "."
    import re
    for prt in default_circuit.parts:          # deterministic tags -> stable, diffable netlist
        prt.tag = prt.ref
    ERC()
    generate_netlist(file_=f"{out}/voiceover_pcb.net")

    # BOM grouped by value + footprint
    groups = {}
    for p in default_circuit.parts:
        key = (p.value, p.footprint)
        groups.setdefault(key, []).append(p.ref)
    with open(f"{out}/voiceover_pcb_bom.csv", "w", newline="") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(["Qty", "Refs", "Value", "Footprint"])
        nat = lambda r: (re.sub(r"\d", "", r), int(re.sub(r"\D", "", r) or 0))
        for (val, fp), refs in sorted(groups.items(), key=lambda kv: nat(min(kv[1], key=nat))):
            w.writerow([len(refs), " ".join(sorted(refs, key=nat)), val, fp])
    print(f"parts={len(default_circuit.parts)} nets={len(default_circuit.nets)}")
