..\build\Windows\protoc --cpp_out=dllexport_decl=FUNAPI_API:..\..\Source\Funapi\Public ^
funapi\management\maintenance_message.proto ^
funapi\network\fun_message.proto ^
funapi\network\ping_message.proto ^
funapi\service\multicast_message.proto ^
funapi\service\redirect_message.proto

..\build\Windows\protoc --cpp_out=..\..\..\..\Source\funapi_plugin_ue4 test_messages.proto

copy ..\..\Source\Funapi\Public\funapi\management\*.cc ..\..\Source\Funapi\Private\funapi\management
del ..\..\Source\Funapi\Public\funapi\management\*.cc
copy ..\..\Source\Funapi\Public\funapi\network\*.cc ..\..\Source\Funapi\Private\funapi\network
del ..\..\Source\Funapi\Public\funapi\network\*.cc
copy ..\..\Source\Funapi\Public\funapi\service\*.cc ..\..\Source\Funapi\Private\funapi\service
del ..\..\Source\Funapi\Public\funapi\service\*.cc

pause
