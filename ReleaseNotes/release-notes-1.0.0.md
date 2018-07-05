# QNRTCWin Release Notes for 1.0.0

## 简介
QNRTCWin 是七牛云推出的一款适用于 Windows 平台的实时音视频 SDK，提供了灵活的接口，支持高度定制以及二次开发。

## 版本
- 发布 QNRTCStreamingD.lib
- 发布 QNRTCStreamingD.dll
- 发布 QNRTCStreaming.lib
- 发布 QNRTCStreaming.dll

## 功能
- 支持导入外部 PCM 音频数据
- 支持导入外部 RAW 视频数据
- 支持屏幕、窗口的采集
- 支持视频渲染镜像功能

## 缺陷
- 修复部分摄像头采集失败的问题
- 修复 Surface Pro 设备打开摄像头崩溃的问题
- 修复某些场景下音频设备枚举失败的问题

## 注意事项
- 本 SDK 基于 `Microsoft Visual Studio 2015` 进行开发，使用了运行时库的多线程静态版本，即 `MTD/MT` 配置，目前提供 Win32 平台架构的开发包。
