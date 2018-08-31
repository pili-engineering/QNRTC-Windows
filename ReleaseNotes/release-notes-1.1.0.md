# QNRTCWin Release Notes for 1.1.0

## 简介
QNRTCWin 是七牛云推出的一款适用于 Windows 平台的实时音视频 SDK，提供了灵活的接口，支持高度定制以及二次开发。

## 版本
- 发布 libcurl.dll
- 发布 QNRTCStreamingD.lib
- 发布 QNRTCStreamingD.dll
- 发布 QNRTCStreaming.lib
- 发布 QNRTCStreaming.dll

## 功能
- 支持对指定用户音量的调节接口
- 支持获取指定用户音量的分贝值
- 支持摄像头和屏幕、窗口分离的采集预览
- 升级 WebRtc 内核版本

## 优化

- 优化弱网下的自动重连机制
- 优化连麦失败时

## 缺陷
- 修复弱网下自动重连 ANR 的问题
- 修复因音频设备测试失败导致 Crash 的问题
- 修复某些因网络原因导致连麦失败的问题
- 修复一些偶现的 Crash

## 注意事项
- 新版本 SDK 已将 IDE 升级为 `Microsoft Visual Studio 2017`，不过本 SDK 将保持对 `Microsoft Visual Studio 2015` 的支持，同时 Demo 也将继续使用 `Microsoft Visual Studio 2015` 进行开发；
- 新版本 SDK 新增对 libcurl.dll （支持 SSL）的依赖，如果与开发者使用的 libcurl 动态库产生冲突，则开发者可以直接使用本 SDK 依赖的 libcurl，其 lib 和 接口头文件均存放在 Demo 源码中的 `thirdpart/libcurl` 中，[Demo GitHub 地址](https://github.com/pili-engineering/QNRTC-Windows)。

## 问题反馈 

当你遇到任何问题时，可以通过在 GitHub 的 repo 提交 `issues` 来反馈问题，请尽可能的描述清楚遇到的问题，如果有错误信息也一同附带，并且在 ```Labels``` 中指明类型为 bug 或者其他。 [通过这里查看已有的 issues 和提交 bug](https://github.com/pili-engineering/QNRTC-Windows)

