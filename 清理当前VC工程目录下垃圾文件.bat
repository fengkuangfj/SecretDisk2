@echo off
REM echo "欢迎来到清理VC工程！"

del /a /f /s *.aps *.cache *.err *.error *.exp *.ilk *.log *.opensdf *.pdb *.sdf *.suo *.tlog *.unsuccessfulbuild *.user *.vddklaunch *.wrn 2>nul

for /r . %%d in (.) do rd /s /q "%%d\objchk_wxp_x86" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\objfre_wxp_x86" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\objchk_win7_x86" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\objfre_win7_x86" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\objchk_wnet_x86" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\objfre_wnet_x86" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\objchk_wlh_x86" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\objfre_wlh_x86" 2>nul

for /r . %%d in (.) do rd /s /q "%%d\objchk_win7_amd64" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\objfre_win7_amd64" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\objchk_wnet_amd64" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\objfre_wnet_amd64" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\objchk_wlh_amd64" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\objfre_wlh_amd64" 2>nul

for /r . %%d in (.) do rd /s /q "%%d\objchk_win7_ia64" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\objfre_win7_ia64" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\objchk_wnet_ia64" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\objfre_wnet_ia64" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\objchk_wlh_ia64" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\objfre_wlh_ia64" 2>nul

for /r . %%d in (.) do rd /s /q "%%d\Debug" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\DebugPS" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\Release" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\ReleasePS" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\x64" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\ipch" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\obj" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\kernelbase.pdb" 2>nul

REM pause