# QNRTCWin Release Notes for 1.1.1

## 简介
QNRTCWin 是七牛云推出的一款适用于 Windows7 及以上系统平台的实时音视频 SDK，提供了包含音视频通话、屏幕分享、数据导入、监听等功能，适用于各种直播、教育、医疗等场景，并提供了灵活的开发接口，支持高度定制以及二次开发。

## 注意
本次升级为主版本升级(1.1.1 -> 2.0.0)，为了支持更灵活的连麦控制和更低的资源开销有比较大的重构。请查看我们的[新版文档站](https://doc.qnsdk.com/rtn/winodws/)
 
自 2.0.0 后，我们为了提高用户阅读文档的体验，使用了新的文档站（老文档地址继续保留），新文档站地址 https://doc.qnsdk.com/rtn

体验新版 Demo 请打开 Demo 后点击 “体验新版本（多Track）” 即可体验

## 功能
- 增加支持同时发布多路视频的核心功能

## 优化
- 提高屏幕采集帧率

## 缺陷
- 修复某些摄像头打开失败的问题

## 注意事项
- 2.0.0 以前的接口文件移动到了 qiniu 目录下
- [Demo GitHub 地址](https://github.com/pili-engineering/QNRTC-Windows)

## 问题反馈 
当你遇到任何问题时，可以通过在 GitHub 的 repo 提交 `issues` 来反馈问题，请尽可能的描述清楚遇到的问题，如果有错误信息也一同附带，并且在 ```Labels``` 中指明类型为 bug 或者其他。 [通过这里查看已有的 issues 和提交 bug](https://github.com/pili-engineering/QNRTC-Windows)
