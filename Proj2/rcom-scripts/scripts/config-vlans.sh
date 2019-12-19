enable
8nortel

#tuxY
#Y=$[Pcnumber]

configure terminal
vlan ${Y}0

interface fastethernet 0/1
switchport mode access
switchport access vlan ${Y}0

interface fastethernet 0/4
switchport mode access
switchport access vlan ${Y}0

vlan ${Y}1
interface fastethernet 0/2
switchport mode access
switchport access vlan ${Y}1

interface fastethernet 0/3
switchport mode access
switchport access vlan ${Y}1

interface fastethernet 0/5
switchport mode access
switchport access vlan ${Y}1

end

#0/Port

#0/1 -> tux1 eth0 to vlan0
#0/4 -> tux4 eth0 to vlan0

#0/2 -> tux2 eth0 to vlan1
#0/3 -> tux4 eth1 to vlan1

#0/5 -> Router GE0/0 to vlan1