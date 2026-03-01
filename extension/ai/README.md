# DuckDB AI Extension

## Overview
多模态 AI 算子扩展，为 DuckDB 添加图像/Embedding/Audio 处理能力。

## Features
- **AI_filter**: 基于模型推理的行过滤（CLIP 相似度搜索）
- **AI_aggregation**: 聚类算法（K-Means）
- **AI_transform**: 数据转换（图像 → Embedding）

## Dependencies
- httplib: HTTP/HTTPS client
- mbedtls: TLS support

## M0 Status
- ✅ Environment verified
- ✅ API design approved
- ⏳ HTTP PoC in progress
