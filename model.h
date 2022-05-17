#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"

class Model {
private:
	std::vector<Vec3f> verts_;
	std::vector<Vec3f> tex_;
	std::vector<std::pair<std::vector<int>, std::vector<int>>> faces_;
public:
	Model(const char* filename);
	~Model();
	int nverts();
	int nfaces();
	Vec3f vert(int i);
	std::pair<std::vector<int>, std::vector<int>> face(int idx);
	Vec3f tex(int i);
};

#endif //__MODEL_H__
