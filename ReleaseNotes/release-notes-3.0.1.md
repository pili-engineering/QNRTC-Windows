# QNRTCWin Release Notes for 3.0.1

## 简介
QNRTCWin 是七牛云推出的一款适用于 Windows7 及以上系统平台的实时音视频 SDK，提供了包含音视频通话、屏幕分享、数据导入、监听等功能，适用于各种直播、教育、医疗等场景，并提供了灵活的开发接口，支持高度定制以及二次开发。

## 注意

自 2.0.0 后，我们为了提高用户阅读文档的体验，使用了新的文档站（老文档地址继续保留），新文档站地址 https://doc.qnsdk.com/rtn/windows

体验新版 Demo 请点击 “体验 DEMO” 即可体验

## 功能
   - 支持摄像头切换图片连麦
   - 支持添加自定义SEI 功能
   - 支持合流时单独设置 Track 填充模式
   - 支持设置渲染窗口画面填充模式
   - 支持对外部视频进行裁剪和缩放功能
   - 支持摄像头采集数据镜像功能

## 缺陷
   - 修复外部导入音频偶现的卡顿问题
   - 修复摄像头预览分辨率设置不正确的问题

## 注意事项
- 2.0.0 以前的接口文件移动到了 qiniu 目录下
- 2.1.0 以后不再提供 Debug 版本的动态库
- [Demo 源码 GitHub 地址](https://github.com/pili-engineering/QNRTC-Windows)
- [Demo 体验下载地址](https://sdk-release.qnsdk.com/Windows-RTC-3.0.1.zip) 

## 问题反馈 
当你遇到任何问题时，可以通过在 GitHub 的 repo 提交 `issues` 来反馈问题，请尽可能的描述清楚遇到的问题，如果有错误信息也一同附带，并且在 ```Labels``` 中指明类型为 bug 或者其他。 [通过这里查看已有的 issues 和提交 bug](https://github.com/pili-engineering/QNRTC-Windows)
