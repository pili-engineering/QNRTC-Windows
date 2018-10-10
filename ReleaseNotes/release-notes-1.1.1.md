# QNRTCWin Release Notes for 1.1.1

## 简介
QNRTCWin 是七牛云推出的一款适用于 Windows 平台的实时音视频 SDK，提供了灵活的接口，支持高度定制以及二次开发。

## 版本
- 发布 QNRTCStreamingD.lib
- 发布 QNRTCStreamingD.dll
- 发布 QNRTCStreaming.lib
- 发布 QNRTCStreaming.dll

## 功能
- 新增音频监听功能
- 新增图像裁减功能（目前仅支持 I420 格式）
- 新增图像格式转换到 I420 的功能
- 新增 D3D 视频渲染方式，默认为 D3D 渲染

## 优化
- 优化接口回调机制

## 缺陷
- 修复某些声卡存在的回音问题
- 修复一些偶现的 Bug

## 注意事项
- SetRoomListener 接口非线程安全的接口，需要在 JoinRoom 之前 和 Leaveroom 之后调用
- [Demo GitHub 地址](https://github.com/pili-engineering/QNRTC-Windows)

## 问题反馈 
当你遇到任何问题时，可以通过在 GitHub 的 repo 提交 `issues` 来反馈问题，请尽可能的描述清楚遇到的问题，如果有错误信息也一同附带，并且在 ```Labels``` 中指明类型为 bug 或者其他。 [通过这里查看已有的 issues 和提交 bug](https://github.com/pili-engineering/QNRTC-Windows)
