from collections import defaultdict
from skidl import Pin, Part, Alias, SchLib, SKIDL, TEMPLATE

from skidl.pin import pin_types

SKIDL_lib_version = '0.0.1'

voiceover_pcb = SchLib(tool=SKIDL).add_parts(*[
        Part(**{ 'name':'Pico_Socket_1x20', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'Pico_Socket_1x20'}), 'ref_prefix':'J', 'fplist':None, 'footprint':'Connector_PinSocket_2.54mm:PinSocket_1x20_P2.54mm_Vertical', 'keywords':None, 'description':'', 'datasheet':None, 'pins':[
            Pin(num='1',name='P1',func=pin_types.PASSIVE),
            Pin(num='2',name='P2',func=pin_types.PASSIVE),
            Pin(num='3',name='P3',func=pin_types.PASSIVE),
            Pin(num='4',name='P4',func=pin_types.PASSIVE),
            Pin(num='5',name='P5',func=pin_types.PASSIVE),
            Pin(num='6',name='P6',func=pin_types.PASSIVE),
            Pin(num='7',name='P7',func=pin_types.PASSIVE),
            Pin(num='8',name='P8',func=pin_types.PASSIVE),
            Pin(num='9',name='P9',func=pin_types.PASSIVE),
            Pin(num='10',name='P10',func=pin_types.PASSIVE),
            Pin(num='11',name='P11',func=pin_types.PASSIVE),
            Pin(num='12',name='P12',func=pin_types.PASSIVE),
            Pin(num='13',name='P13',func=pin_types.PASSIVE),
            Pin(num='14',name='P14',func=pin_types.PASSIVE),
            Pin(num='15',name='P15',func=pin_types.PASSIVE),
            Pin(num='16',name='P16',func=pin_types.PASSIVE),
            Pin(num='17',name='P17',func=pin_types.PASSIVE),
            Pin(num='18',name='P18',func=pin_types.PASSIVE),
            Pin(num='19',name='P19',func=pin_types.PASSIVE),
            Pin(num='20',name='P20',func=pin_types.PASSIVE)] }),
        Part(**{ 'name':'Conn_XH_2', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'Conn_XH_2'}), 'ref_prefix':'J', 'fplist':None, 'footprint':'Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical', 'keywords':None, 'description':'', 'datasheet':None, 'pins':[
            Pin(num='1',name='SIG',func=pin_types.PASSIVE),
            Pin(num='2',name='GND',func=pin_types.PASSIVE)] }),
        Part(**{ 'name':'Conn_XH_4', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'Conn_XH_4'}), 'ref_prefix':'J', 'fplist':None, 'footprint':'Connector_JST:JST_XH_B4B-XH-A_1x04_P2.50mm_Vertical', 'keywords':None, 'description':'', 'datasheet':None, 'pins':[
            Pin(num='1',name='VCC',func=pin_types.PASSIVE),
            Pin(num='2',name='GND',func=pin_types.PASSIVE),
            Pin(num='3',name='A',func=pin_types.PASSIVE),
            Pin(num='4',name='B',func=pin_types.PASSIVE)] }),
        Part(**{ 'name':'SolderJumper_3', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'SolderJumper_3'}), 'ref_prefix':'JP', 'fplist':None, 'footprint':'Jumper:SolderJumper-3_P1.3mm_Open_RoundedPad1.0x1.5mm', 'keywords':None, 'description':'', 'datasheet':None, 'pins':[
            Pin(num='1',name='A',func=pin_types.PASSIVE),
            Pin(num='2',name='C',func=pin_types.PASSIVE),
            Pin(num='3',name='B',func=pin_types.PASSIVE)] }),
        Part(**{ 'name':'C', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'C'}), 'ref_prefix':'C', 'fplist':None, 'footprint':'Capacitor_SMD:C_0805_2012Metric', 'keywords':None, 'description':'', 'datasheet':None, 'pins':[
            Pin(num='1',name='1',func=pin_types.PASSIVE),
            Pin(num='2',name='2',func=pin_types.PASSIVE)] }),
        Part(**{ 'name':'R', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'R'}), 'ref_prefix':'R', 'fplist':None, 'footprint':'Resistor_SMD:R_0805_2012Metric', 'keywords':None, 'description':'', 'datasheet':None, 'pins':[
            Pin(num='1',name='1',func=pin_types.PASSIVE),
            Pin(num='2',name='2',func=pin_types.PASSIVE)] }),
        Part(**{ 'name':'Buzzer', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'Buzzer'}), 'ref_prefix':'BZ', 'fplist':None, 'footprint':'Buzzer_Beeper:Buzzer_12x9.5RM7.6', 'keywords':None, 'description':'', 'datasheet':None, 'pins':[
            Pin(num='1',name='+',func=pin_types.PASSIVE),
            Pin(num='2',name='-',func=pin_types.PASSIVE)] }),
        Part(**{ 'name':'Q_NPN_BEC', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'Q_NPN_BEC'}), 'ref_prefix':'Q', 'fplist':None, 'footprint':'Package_TO_SOT_SMD:SOT-23', 'keywords':None, 'description':'', 'datasheet':None, 'pins':[
            Pin(num='1',name='B',func=pin_types.PASSIVE),
            Pin(num='2',name='E',func=pin_types.PASSIVE),
            Pin(num='3',name='C',func=pin_types.PASSIVE)] }),
        Part(**{ 'name':'D_Small', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'D_Small'}), 'ref_prefix':'D', 'fplist':None, 'footprint':'Diode_SMD:D_SOD-323', 'keywords':None, 'description':'', 'datasheet':None, 'pins':[
            Pin(num='1',name='K',func=pin_types.PASSIVE),
            Pin(num='2',name='A',func=pin_types.PASSIVE)] }),
        Part(**{ 'name':'Conn_PH_2', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'Conn_PH_2'}), 'ref_prefix':'J', 'fplist':None, 'footprint':'Connector_JST:JST_PH_B2B-PH-K_1x02_P2.00mm_Vertical', 'keywords':None, 'description':'', 'datasheet':None, 'pins':[
            Pin(num='1',name='+',func=pin_types.PASSIVE),
            Pin(num='2',name='-',func=pin_types.PASSIVE)] }),
        Part(**{ 'name':'Conn_XH_2_Switch', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'Conn_XH_2_Switch'}), 'ref_prefix':'J', 'fplist':None, 'footprint':'Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical', 'keywords':None, 'description':'', 'datasheet':None, 'pins':[
            Pin(num='1',name='T1',func=pin_types.PASSIVE),
            Pin(num='2',name='T2',func=pin_types.PASSIVE)] }),
        Part(**{ 'name':'D_Schottky', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'D_Schottky'}), 'ref_prefix':'D', 'fplist':None, 'footprint':'Diode_SMD:D_SOD-123', 'keywords':None, 'description':'', 'datasheet':None, 'pins':[
            Pin(num='1',name='K',func=pin_types.PASSIVE),
            Pin(num='2',name='A',func=pin_types.PASSIVE)] }),
        Part(**{ 'name':'MCP73831', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'MCP73831'}), 'ref_prefix':'U', 'fplist':None, 'footprint':'Package_TO_SOT_SMD:SOT-23-5', 'keywords':None, 'description':'', 'datasheet':None, 'pins':[
            Pin(num='1',name='STAT',func=pin_types.PASSIVE),
            Pin(num='2',name='VSS',func=pin_types.PASSIVE),
            Pin(num='3',name='VBAT',func=pin_types.PASSIVE),
            Pin(num='4',name='VDD',func=pin_types.PASSIVE),
            Pin(num='5',name='PROG',func=pin_types.PASSIVE)] }),
        Part(**{ 'name':'LED', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'LED'}), 'ref_prefix':'D', 'fplist':None, 'footprint':'LED_SMD:LED_0805_2012Metric', 'keywords':None, 'description':'', 'datasheet':None, 'pins':[
            Pin(num='1',name='K',func=pin_types.PASSIVE),
            Pin(num='2',name='A',func=pin_types.PASSIVE)] }),
        Part(**{ 'name':'Header_2', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'Header_2'}), 'ref_prefix':'J', 'fplist':None, 'footprint':'Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical', 'keywords':None, 'description':'', 'datasheet':None, 'pins':[
            Pin(num='1',name='1',func=pin_types.PASSIVE),
            Pin(num='2',name='2',func=pin_types.PASSIVE)] }),
        Part(**{ 'name':'MountingHole', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'MountingHole'}), 'ref_prefix':'H', 'fplist':None, 'footprint':'MountingHole:MountingHole_3.2mm_M3', 'keywords':None, 'description':'', 'datasheet':None })])