#include "EulerOperations.h"
#include <iostream>

Vertex* EulerOperations::mvfs(const Point& _p)
{
    std::cout << "[DEBUG] mvfs操作开始，创建初始顶点" << std::endl;
    
    // 创建顶点
    Vertex* v = new Vertex{_p, nullptr};
    if (!v) {
        std::cout << "[ERROR] mvfs: 无法创建顶点" << std::endl;
        return nullptr;
    }
    
    // 创建体
    if (body_ != nullptr)
    {
        delete body_;
    }
    body_ = new Body;
    if (!body_) {
        std::cout << "[ERROR] mvfs: 无法创建体" << std::endl;
        delete v;
        return nullptr;
    }
    // 显式初始化所有计数为0
    body_->face_num_ = 0;
    body_->edge_num_ = 0;
    body_->vertex_num_ = 0;
    
    // 创建面
    Face* face = new Face;
    if (!face) {
        std::cout << "[ERROR] mvfs: 无法创建面" << std::endl;
        delete v;
        delete body_;
        body_ = nullptr;
        return nullptr;
    }
    face->body_ = body_;
    face->next_face_ = nullptr;
    face->prev_face_ = nullptr;
    body_->first_face_ = face;
    body_->face_num_ = 1;
    
    // 创建外环
    Loop* loop = new Loop;
    if (!loop) {
        std::cout << "[ERROR] mvfs: 无法创建环" << std::endl;
        delete v;
        delete face;
        delete body_;
        body_ = nullptr;
        return nullptr;
    }
    loop->face_ = face;
    loop->next_loop_ = nullptr;
    loop->prev_loop_ = nullptr;
    face->first_loop_ = loop;
    
    // 设置顶点数为1
    body_->vertex_num_ = 1;
    
    std::cout << "[DEBUG] mvfs操作完成，成功创建顶点" << std::endl;
    return v;
}

Halfedge* EulerOperations::mev(Vertex* _v0, Vertex* _v1, Loop* _loop)
{
    if (!_v0 || !_v1 || !_loop) return nullptr;
    
    std::cout << "mev操作开始，连接顶点..." << std::endl;
    
    // 创建新边及其两个半边
    Halfedge* he0 = new Halfedge;
    Halfedge* he1 = new Halfedge;
    Edge* edge = new Edge;

    // 设置边和半边的关系
    he0->edge_ = edge;
    he1->edge_ = edge;
    edge->he0_ = he0;
    edge->he1_ = he1;

    // 设置环的关系
    he0->loop_ = _loop;
    he1->loop_ = _loop;

    // 设置顶点关系
    he0->start_vertex_ = _v0;
    he0->to_vertex_ = _v1;
    he1->start_vertex_ = _v1;
    he1->to_vertex_ = _v0;

    // 设置对边关系
    he0->oppo_he_ = he1;
    he1->oppo_he_ = he0;

    // 初始化其他字段
    he0->next_he_ = nullptr;
    he0->prev_he_ = nullptr;
    he1->next_he_ = nullptr;
    he1->prev_he_ = nullptr;

    // 更新顶点的半边指针
    _v0->he_ = he0;
    _v1->he_ = he1;

    // 如果环为空环（没有起始半边）
    if (_loop->start_he_ == nullptr)
    {
        std::cout << "mev: 环为空，创建新环..." << std::endl;
        // 形成一个环
        he0->next_he_ = he1;
        he1->next_he_ = he0;
        he0->prev_he_ = he1;
        he1->prev_he_ = he0;
        _loop->start_he_ = he0;
    }
    else
    {
        std::cout << "mev: 向现有环中插入边..." << std::endl;
        // 查找以_v0为终点的半边
        Halfedge* he = _loop->start_he_;
        bool found = false;
        
        if (he) {
            do {
                if (he->to_vertex_ == _v0) {
                    found = true;
                    break;
                }
                he = he->next_he_;
            } while (he != _loop->start_he_);
        }
    
        // 如果找到正确的半边
        if (found && he) {
            // 插入新的半边到环中
            he0->next_he_ = he1;
            he0->prev_he_ = he;

            if (he->next_he_) {
                he1->next_he_ = he->next_he_;
                he1->prev_he_ = he0;

                he->next_he_->prev_he_ = he1;
                he->next_he_ = he0;
            } else {
                std::cout << "mev: 环结构不完整" << std::endl;
                delete he0;
                delete he1;
                delete edge;
                return nullptr;
            }
        } else {
            std::cout << "mev: 未找到正确的插入位置" << std::endl;
            // 清理资源
            delete he0;
            delete he1;
            delete edge;
            return nullptr;
        }
    }

    // 更新体的边信息
    body_->edges_.push_back(edge);
    body_->edge_num_++;
    body_->vertex_num_++;

    std::cout << "mev: 操作完成" << std::endl;
    return he0;
}

Loop* EulerOperations::mef(Vertex* _v0, Vertex* _v1, Loop* _lp)
{
    if (!_v0 || !_v1 || !_lp) return nullptr;
    
    std::cout << "mef操作开始..." << std::endl;
    
    // 查找连接_v0和_v1的边
    Halfedge* he = _lp->start_he_;
    Halfedge* target_he = nullptr;
    
    // 遍历环查找对应的半边
    if (he) {
        do {
            if (he->start_vertex_ == _v0 && he->to_vertex_ == _v1) {
                target_he = he;
                break;
            }
            he = he->next_he_;
        } while (he != _lp->start_he_);
    }
    
    // 如果没有找到对应的半边，先使用mev创建
    if (target_he == nullptr) {
        std::cout << "mef: 未找到边，调用mev创建..." << std::endl;
        target_he = mev(_v0, _v1, _lp);
        if (!target_he) {
            std::cout << "mef: mev调用失败" << std::endl;
            return nullptr;
        }
    }
    
    // 创建新面
    std::cout << "mef: 创建新面..." << std::endl;
    Face* new_face = new Face;
    new_face->body_ = body_;
    new_face->next_face_ = nullptr;
    new_face->prev_face_ = nullptr;
    new_face->first_loop_ = nullptr;
    
    // 创建新环
    std::cout << "mef: 创建新环..." << std::endl;
    Loop* new_loop = new Loop;
    new_loop->face_ = new_face;
    new_loop->next_loop_ = nullptr;
    new_loop->prev_loop_ = nullptr;
    new_loop->start_he_ = nullptr;
    
    // 设置新面的第一个环
    new_face->first_loop_ = new_loop;
    
    // 将新面添加到体的面列表中
    if (body_->first_face_) {
        new_face->next_face_ = body_->first_face_;
        body_->first_face_->prev_face_ = new_face;
    }
    body_->first_face_ = new_face;
    
    // 分割原环
    Halfedge* he1 = target_he;
    Halfedge* he2 = nullptr;
    
    if (he1 && he1->oppo_he_) {
        he2 = he1->oppo_he_;
    } else {
        std::cout << "mef: 找不到对边" << std::endl;
        delete new_face;
        delete new_loop;
        return nullptr;
    }
    
    // 更新原环的起始半边
    _lp->start_he_ = he1->next_he_;
    
    // 设置新环的起始半边
    new_loop->start_he_ = he2->next_he_;
    
    // 更新面的环信息
    _lp->face_->body_ = body_;
    
    // 更新体的面数
    body_->face_num_++;
    
    std::cout << "mef: 操作完成" << std::endl;
    return new_loop;
}

Loop* EulerOperations::kemr(Vertex* _v0, Vertex* _v1, Loop* _lp)
{
    if (!_v0 || !_v1 || !_lp) return nullptr;
    
    // 查找连接_v0和_v1的边
    Halfedge* he = _lp->start_he_;
    Halfedge* target_he = nullptr;
    
    while (he) {
        if ((he->start_vertex_ == _v0 && he->to_vertex_ == _v1) ||
            (he->start_vertex_ == _v1 && he->to_vertex_ == _v0)) {
            target_he = he;
            break;
        }
        he = he->next_he_;
        if (he == _lp->start_he_) break;
    }
    
    if (!target_he) return nullptr;
    
    Halfedge* oppo_he = target_he->oppo_he_;
    if (!oppo_he) return nullptr;
    
    Edge* edge = target_he->edge_;
    if (!edge) return nullptr;
    
    // 创建内环
    Loop* inner_loop = new Loop;
    inner_loop->face_ = _lp->face_;
    inner_loop->next_loop_ = nullptr;
    inner_loop->prev_loop_ = nullptr;
    
    // 保存需要的半边
    Halfedge* prev_he = target_he->prev_he_;
    Halfedge* next_he = oppo_he->next_he_;
    Halfedge* oppo_prev_he = oppo_he->prev_he_;
    
    if (!prev_he || !next_he || !oppo_prev_he) {
        delete inner_loop;
        return nullptr;
    }
    
    // 重新连接外环的半边
    prev_he->next_he_ = next_he;
    next_he->prev_he_ = prev_he;
    
    // 设置内环的起始半边
    inner_loop->start_he_ = oppo_prev_he;
    
    // 插入内环到面的环列表中
    inner_loop->next_loop_ = _lp->face_->first_loop_;
    if (_lp->face_->first_loop_) {
        _lp->face_->first_loop_->prev_loop_ = inner_loop;
    }
    _lp->face_->first_loop_ = inner_loop;
    
    // 从体的边列表中删除该边
    body_->delete_edge(edge);
    body_->edge_num_ = std::max(0, body_->edge_num_ - 1);
    
    return inner_loop;
}

void EulerOperations::kfmrh(Loop* _out_loop, Loop* _loop)
{
    if (!_out_loop || !_loop) return;
    
    // 确保两个环属于不同的面
    if (_out_loop->face_ == _loop->face_) return;
    
    // 将_loop从原来的面中移除
    if (_loop->prev_loop_) {
        _loop->prev_loop_->next_loop_ = _loop->next_loop_;
    }
    if (_loop->next_loop_) {
        _loop->next_loop_->prev_loop_ = _loop->prev_loop_;
    }
    
    // 如果_loop是原面的第一个环
    if (_loop->face_->first_loop_ == _loop) {
        _loop->face_->first_loop_ = _loop->next_loop_;
    }
    
    // 将_loop添加为_out_loop所在面的内环
    _loop->face_ = _out_loop->face_;
    _loop->next_loop_ = _out_loop->face_->first_loop_;
    if (_out_loop->face_->first_loop_) {
        _out_loop->face_->first_loop_->prev_loop_ = _loop;
    }
    _loop->prev_loop_ = nullptr;
    _out_loop->face_->first_loop_ = _loop;
    
    // 减少体的面数
    body_->face_num_ = std::max(0, body_->face_num_ - 1);
}
