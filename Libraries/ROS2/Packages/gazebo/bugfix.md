# bugfix

本文记录遇见的bug.

```shell
The following packages have unmet dependencies:
 gz-tools2 : Conflicts: gazebo (>= 11.0.0) but 11.10.2+dfsg-1 is to be installed
             Conflicts: gazebo (<= 11.14.0) but 11.10.2+dfsg-1 is to be installed
E: Error, pkgProblemResolver::Resolve generated breaks, this may be caused by held packages.
```

遇到`gz-tools2`有无法完成的依赖。

`gz-tools2`是`gazebo classic`的包，直接删除。

```shell
sudo apt remove gz-tools2
```
