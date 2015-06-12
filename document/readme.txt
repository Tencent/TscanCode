=========
TscanCode
=========


关于

    TscanCode是基于CPPCheck对c/c++代码实现静态代码扫描解决方案。遵循GPLV3开源协议

手册

   目录下TscanCode用户手册.Doc

编译

   
      * Windows: Visual Studio 2010和以上版本
      * Linux:	 Gcc4.4.4+(推荐Gcc4.4.6)

    Visual Studio
    =============
        打开 TscanCode_vs2010.sln 文件
		右键 TscanCode_vs2010解决方案重新生成即可
		
    gcc
    =================
       拷贝工程到linux服务器上,cd进入到makefile所在路径，直接执行make即可。
	   make将直接生成TscanCode二进制程序于当前目录
执行
	调用方式:以下路径均为系统绝对路径。
	TscanCode --xml src(扫描全路径) --writefile=输出结果路径/结果文件xml --configpath=所在路径/TscanCodeConfig 
   
目录结构
	trunk/cli/: 					控制台程序文件
	trunk/common/:		 		使用到的common库文件
	trunk/externals/				引用第三方库文件,tinyxml
	trunk/lib/					cppcheck lib文件，添加修改的扫描文件等。
	release/linux_bin/				在linux下编译通过的二进制文件
	release/samples/				简单的错误cpp示例，用于扫描报错Demo
	release/TscanCOdeConfig/		TscanCode配置文件
	release/windows_console_bin/
	document/cppchekreadme/ 		cppchek相关的readme和捐献文档说明
	document/TscanCode开源文档/		TscanCode用户手册等说明文档	
支持网站
    待定。