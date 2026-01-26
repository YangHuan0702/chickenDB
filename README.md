# 🚀 一个从零实现的 OLAP 数据库（DuckDB 风格）

> **目标**：构建一个真正“懂硬件、懂查询、懂分析”的单机 OLAP 数据库内核，而不是拼 API 的玩具项目。

这是一个**从零开始**实现的 OLAP 数据库项目，设计理念深受 **DuckDB / MonetDB / ClickHouse（单机部分）** 启发，但并不照抄实现，而是力图在 *可理解性、可扩展性、工程真实度* 之间取得平衡。

如果你也对以下问题感兴趣，那么这个项目大概率对你有价值：

* 查询优化器到底在“优化”什么？
* 向量化执行为什么这么快？
* OLAP 数据库如何榨干 CPU Cache / SIMD / 内存带宽？
* 从 SQL 到算子执行，中间每一层真实长什么样？

---

## ✨ 核心特性（Core Features）

### 🧠 查询引擎

* **SQL → Logical Plan → Physical Plan → Pipeline 执行模型**
* 明确区分：

    * *Logical Operator*：我要做什么（Scan / Filter / Aggregate / Join）
    * *Physical Operator*：我怎么做（HashAgg / SortAgg / HashJoin / NLJ）
* 基于 **成本模型（Cost Model）** 的规则 + 代价混合优化

### ⚡ 向量化执行（Vectorized Execution）

* 批处理（Block / Chunk / Vector）驱动，而非 tuple-at-a-time
* 减少函数调用、分支预测失败、cache miss
* 为 SIMD / 列式布局做好准备

```text
for chunk in table:
    filter(chunk)
    project(chunk)
    aggregate(chunk)
```

> 设计目标：让 CPU 一直在“算”，而不是在等内存。

---

### 🗂️ 存储引擎（Storage Engine）

* **列式存储（Columnar Storage）**
* 数据组织：

    * Table → Segment → Block → Column
* 支持：

    * Late Materialization
    * Null Bitmap
    * 定长 / 变长数据分离

> 明确偏向 **OLAP workload**，不追求高并发写入。

---

### 📊 聚合与 Join

* Hash Aggregate / Sort Aggregate
* Hash Join（Build / Probe 分离）
* Join 顺序可由优化器决定

重点不是“功能全”，而是：

> **每一种算法都能解释清楚：为什么要这么写？**

---

## 🧱 系统架构（Architecture）

```text
        SQL
         │
         ▼
   ┌────────────┐
   │   Parser   │
   └────────────┘
         │ AST
         ▼
   ┌────────────┐
   │ Binder     │  ← 名字解析 / 类型推导
   └────────────┘
         │
         ▼
   ┌────────────┐
   │ Logical    │  ← 关系代数
   │ Optimizer  │
   └────────────┘
         │
         ▼
   ┌────────────┐
   │ Physical   │  ← 算法选择
   │ Planner    │
   └────────────┘
         │
         ▼
   ┌────────────┐
   │ Execution  │  ← Vectorized / Pipeline
   │ Engine     │
   └────────────┘
```

---

## 🛠️ 技术选型（Tech Stack）

* **语言**：C++（偏现代 C++，关注可读性）
* **构建系统**：CMake
* **测试**：单元测试 + 查询级测试
* **目标平台**：Linux / macOS（单机）

> 不依赖第三方大数据库内核，核心逻辑尽量自研。

---

## 📚 设计原则（Design Philosophy）

### 1️⃣ 可解释性优先

> 每一行核心代码，都应该能回答：**“为什么数据库要这样做？”**

不是黑盒性能秀，而是白盒系统学习工程。

### 2️⃣ 尊重硬件现实

* Cache line
* 顺序读 > 随机读
* 分支预测
* SIMD / 向量化

这是一个**为 CPU 而设计的数据库**。

### 3️⃣ 不追求“全”，追求“对”

* 不做分布式（至少现在不做）
* 不追求 ACID 完整事务
* 专注 **OLAP 查询路径**

---

## 🧪 示例（Example）

```sql
SELECT
    region,
    SUM(revenue)
FROM orders
WHERE order_date >= '2024-01-01'
GROUP BY region;
```

执行路径大致为：

```text
SeqScan → Filter → HashAggregate → Result
```

整个过程以 **Vector / Chunk** 为单位流动。

---

## 🎯 适合谁？

* 想**真正理解数据库内核**的人
* 读过《DDIA》《CMU 15-721》《DuckDB 源码》但仍觉得不够“落地”的人
* 想从 **工程角度** 系统性掌握 OLAP 的人

不太适合：

* 只想快速造轮子
* 只关心 SQL 功能，不关心实现

---

## 🧠 致敬与参考（Acknowledgements）

* DuckDB
* MonetDB
* CMU 15-721 / 15-445
* 《Designing Data-Intensive Applications》
* 《Database Internals》

---

## 📌 项目状态

> 🚧 **Active Development**

这是一个持续演进的系统级项目，而不是 Demo。

如果你对数据库内核、查询执行、向量化、优化器感兴趣——

**Welcome aboard.** 😄
