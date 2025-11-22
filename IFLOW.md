# digraphx-cpp 项目综合指南

## 项目概述

digraphx-cpp 是一个受 NetworkX 启发的现代 C++ 项目，专注于有向图算法的实现。该项目提供了负环检测、最小环比率计算、参数化网络问题求解等功能。项目采用现代 CMake 实践，支持单头文件库和各种规模的项目，具有清晰的库与可执行代码分离、集成的测试套件和持续集成等功能。

## 核心功能

- **负环检测** (`neg_cycle.hpp`): 实现了基于 Howard 方法的负环检测算法，使用策略迭代算法找到有向图的最小环比率。
- **最小环比率求解** (`min_cycle_ratio.hpp`): 解决有向图中具有最小比率（边权重总和与边数的比率）的环的问题。
- **参数化问题求解** (`parametric.hpp`): 解决网络参数化问题，最大化参数 r，满足距离约束。
- **带约束的负环检测** (`neg_cycle_q.hpp`): 实现了带约束的 Howard 算法，支持前驱和后继两种版本。
- **带约束的最小参数化求解** (`min_parametric_q.hpp`): 解决带约束的参数化网络问题。

## 项目结构

- `include/digraphx/`: 包含所有头文件，定义了主要的算法类和函数
- `source/`: 包含源文件（当前只有 dummy.cpp）
- `test/`: 包含测试文件
- `cmake/`: 包含 CMake 配置文件
- `standalone/`: 独立可执行文件的构建配置
- `documentation/`: 文档生成配置

## 依赖管理

项目使用 CPM.cmake 进行依赖管理，包含以下主要依赖：
- fmt: 格式化库
- Py2Cpp: 用于 Python 风格的 C++ 实用工具
- MyWheel: 项目特定的轮子库

## 构建与运行

### 构建独立可执行文件
```bash
cmake -S standalone -B build/standalone
cmake --build build/standalone
./build/standalone/DiGraphX --help
```

### 构建并运行测试套件
```bash
cmake -S test -B build/test
cmake --build build/test
CTEST_OUTPUT_ON_FAILURE=1 cmake --build build/test --target test
```

### 一次性构建所有内容
```bash
cmake -S all -B build
cmake --build build
```

### 代码格式化
```bash
# 查看格式更改
cmake --build build/test --target format

# 应用格式更改
cmake --build build --target fix-format
```

### 构建文档
```bash
cmake -S documentation -B build/doc
cmake --build build/doc --target GenerateDocs
```

## 编码约定

- 使用 C++20 标准
- 代码格式遵循 clang-format 和 cmake-format
- 支持多种静态分析工具（clang-tidy, iwyu, cppcheck）
- 使用 sanitizer 工具进行内存和未定义行为检测

## 主要接口和类

- `NegCycleFinder`: 负环检测器，使用 Howard 方法
- `MinCycleRatioSolver`: 最小环比率求解器
- `MaxParametricSolver`: 最大参数化求解器
- `NegCycleFinderQ`: 带约束的负环检测器
- `MinParametricSolver`: 带约束的最小参数化求解器
- `MapAdapter` / `MapConstAdapter`: 向量容器的字典风格适配器

## 测试

项目包含全面的测试套件，使用 doctest 框架。测试文件位于 `test/source/` 目录下，涵盖了各种图结构表示（列表列表、字典列表等）的负环检测和参数化问题求解。