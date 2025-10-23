#ifndef _EULER_OPERATIONS_H_
#define _EULER_OPERATIONS_H_

#include "SolidModel.h"



class EulerOperations
{
public:
	EulerOperations() {}
	~EulerOperations() 
	{
		if (body_ != nullptr)
		{
			delete body_;
			body_ = nullptr;
		}
	}

	Body* get_body()
	{
		return body_;
	}
public:
	//--- ʵ�����µ�ŷ������ ---//

	Vertex* mvfs(const Point & _p);
	Halfedge* mev(Vertex* _v0, Vertex * _v1, Loop* _lp);
	Loop* mef(Vertex* _v0, Vertex* _v1, Loop* _lp);
	Loop* kemr(Vertex* _v0, Vertex* _v1, Loop* _lp);
	void kfmrh(Loop* _out_loop, Loop* _loop);

private:
	Body* body_ = nullptr;
};


#endif // !_EULER_OPERATIONS_H_