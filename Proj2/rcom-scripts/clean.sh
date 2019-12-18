#SWITCH
configure terminal
no vlan 2-4094
exit
copy flash:tuxy-clean startup-config
reload

#ROUTER
copy flash:tuxy-clean startup-config
reload