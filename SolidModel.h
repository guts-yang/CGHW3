#ifndef _SOLID_MODEL_H_
#define _SOLID_MODEL_H_

#include <vector>
#include <algorithm>


struct Point;
struct Vertex;
struct Halfedge;
struct Edge;
struct Loop;
struct Face;
struct Body;

typedef struct Point
{
	Point(double _x, double _y, double _z)
	{
		coordinate_[0] = _x;
		coordinate_[1] = _y;
		coordinate_[2] = _z;
	}
	Point(const double _coord[3])
	{
		coordinate_[0] = _coord[0];
		coordinate_[1] = _coord[1];
		coordinate_[2] = _coord[2];
	}
	double& operator[] (const int& _idx)
	{
		return coordinate_[_idx];
	}
	const double& operator[] (const int& _idx)const
	{
		return coordinate_[_idx];
	}
	double x()
	{
		return coordinate_[0];
	}
	double y()
	{
		return coordinate_[1];
	}
	double z()
	{
		return coordinate_[2];
	}
	double coordinate_[3] = {0, 0, 0}; // ������������
}Point;

typedef struct Vertex
{
	Point     p_;            // ���㼸��λ��
	Halfedge* he_ = nullptr; // �붥����ص�һ�����
}Vertex;

typedef struct Halfedge
{
	Vertex* start_vertex_   = nullptr;  // ���
	Vertex* to_vertex_ = nullptr;       // to vertex
	Halfedge* next_he_ = nullptr;       // ǰһ�����
	Halfedge* prev_he_ = nullptr;       // ǰһ�����
	Halfedge* oppo_he_ = nullptr;       // ����Ա�
	Edge* edge_        = nullptr;       // ��Ӧ�ı�
	Loop* loop_        = nullptr;       // ������ڵ� Loop
}Halfedge;

typedef struct Edge
{
	Halfedge* he0_;
	Halfedge* he1_;
}Edge;

typedef struct Loop
{
	Halfedge* start_he_ = nullptr; // ��ʼ��
	Loop* next_loop_    = nullptr; // ��һ�� Loop
	Loop* prev_loop_    = nullptr; // ǰһ�� Loop
	Face* face_         = nullptr; // Loop ���ڵ���
}Loop;

typedef struct Face
{
	Loop* first_loop_ = nullptr; // ��һ�� Loop
	Face* next_face_  = nullptr; // ��һ����
	Face* prev_face_  = nullptr; // ǰһ����
	Body* body_       = nullptr; // ��Ӧ�� Body 
}Face;

typedef struct Body
{
	Face* first_face_ = nullptr; // ��һ�� Face 
	std::vector<Edge*> edges_;   // ��¼���еıߣ������߿���ʾ	
	int face_num_ = 0;           // ��ĸ���
	int edge_num_ = 0;           // ����
	int vertex_num_ = 0;         // ������

	/** ɾ����¼�ı� */
	void delete_edge(Edge* _e)
	{
		auto it = std::find(edges_.begin(), edges_.end(), _e);
		if (it != edges_.end())
		{
			if (*it)
			{
				delete* it;
				*it = nullptr;
			}
			edges_.erase(it);
		}
	}


	Body()
	{
		// 确保edges_向量被正确初始化
		edges_.clear();
	}

	~Body()
	{
		// 清理所有边
		for (auto edge : edges_)
		{
			if (edge)
			{
				delete edge;
				edge = nullptr;
			}
		}
		edges_.clear();
	}

}Body;

#endif // !_SOLIDMODEL_H_
