#tuxY
#Y=$[Pcnumber]
#I320=$[fastethernet]
#I321=$[gigabitethernet]

configure terminal

interface gigabitethernet 0/0
ip address 172.16.${Y}1.254 255.255.255.0
no shutdown
ip nat inside
exit

interface gigabitethernet 0/1
ip address 172.16.1.${Y}9 255.255.255.0
no shutdown
ip nat outside
exit

ip nat pool ovrld 172.16.1.${Y}9 172.16.1.${Y}9 prefix 24
ip nat inside source list 1 pool ovrld overload 

access-list 1 permit 172.16.${Y}0.0 0.0.0.7
access-list 1 permit 172.16.${Y}1.0 0.0.0.7 

ip route 0.0.0.0 0.0.0.0 172.16.1.254
ip route 172.16.${Y}0.0 255.255.255.0 172.16.${Y}1.253

end

#access-list 1ºline -> Mudar o 7 para 255 para dar todos os endereços
#0/0 -> Router GE0/0 to switch Port #0/5 -> router to vlan1 (check config-vlans.sh)
#0/1 -> Router GE0/1 to central Port Y.1