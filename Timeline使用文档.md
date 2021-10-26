# Timeline开发规范(使用说明)文档

## 开发流程

1. 先确保Timelines没有时间线，ClearTimelines和ClearSettingList
2. 然后添加一个默认的时间线和设置，GenerateDefaultTimeline
3. 然后把生成后的默认时间线添加到Timelines数组里面，AddTimelineInfo
4. 根据需要使用Add函数添加事件，轨道，并且在需要的地方绑定事件调度器。
5. 确保对应的事件调度器在执行播放之前就绑定完了
6. 播放他！

## 数据结构说明

1. 数据结构都在AdvancedTimelineData.h里面
2. EAdvTimelineType：自定义的类型
3. ELengthMode：时间线的长度模式
4. EPlayStatus：播放状态
5. EPlayMethod：播放方法
6. FKeyInfo: 关键帧基类
7. FCurveInfoBase: 曲线基类
8. FEventKey: 专属于事件曲线的事件关键帧
9. FTrackInfoBase: 轨道基类
10. FTimelineSetting: 时间线配置
11. FTimelineInfo: 时间线本体

## 细节说明

1. 一个项目对应多个时间线
2. 一个时间线对应一个时间线配置
   - 时间线可以像软件切换配置一样切换对应的配置(虽然用户自己存一下也可以实现....)
3. 一个时间线对应多个轨道，但是同步播放，如果需要只播放一个轨道可以把他单独加到一个时间线中....
4. 一个轨道对应一个曲线
5. 一个曲线对应N个关键帧
6. 去除自动播放，在需要播放的时候手动执行播放。
7. UAdvancedTimelineComponent为总控类，属于Controller，管理控制时间线。
   - 其实管理的是单个时间线，但是存放了所有的时间线，因为GuName机制的存在，所以需要根据GuName来操作对应的时间线
   - **GuName因为是用来标识时间线和轨道唯一性的，所以不要重名！**
   - 因为其存放了所有的时间线，所以不要放单个时间线相关的信息。
8. FTimelineInfo结构体存放单个时间线相关的信息。比如所属的轨道和播放状态等
9. FTimelineSetting结构体存放和时间线关联的一些设置，属于和时间线不是强关联的，可以随时更换时间线的一些设置。比如Length，PlayRate。
10. 开始播放的时间总是为0
    - 但是最小的关键帧时间不一定为0
11. 我不推荐直接操作Timelines数组和SettingList。
12. 时间线和轨道的事件使用动态多播委托，在需要的时候请用传入的GuName来判断应该执行哪个函数
13. 事件的绑定应该始终执行。且可以在任何地方绑定
14. 目前添加事件需要直接添加轨道，后续会视情况添加：直接加事件的函数
15. 目前保存加载Sav文件也是可以用的
