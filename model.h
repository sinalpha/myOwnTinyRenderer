#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

struct Vector3d {
	float x;
	float y;
	float z;
};

struct Face {
	int indices[3];
};

struct Model {

	Model(const std::string& fileName) {

		std::ifstream in{ fileName };
		if (not in.is_open()) {
			std::cout << "Failed to open \"" << fileName << "\".";
		}

		std::string line;

		while (std::getline(in, line)) {
			std::stringstream ss(line);
			std::string type;

			ss >> type;

			if ("v" == type) {
				float x, y, z;
				ss >> x >> y >> z;
				mVertexBuffer.emplace_back(Vector3d{ x, y, z });
			}
			else if ("f" == type) {
				std::string vertexIndexInfo[3];

				ss >> vertexIndexInfo[0] >> vertexIndexInfo[1] >> vertexIndexInfo[2];

				mFaceBuffer.emplace_back(Face{
					std::stoi(vertexIndexInfo[0].substr(0, vertexIndexInfo[0].find('/'))) - 1,
					std::stoi(vertexIndexInfo[1].substr(0, vertexIndexInfo[1].find('/'))) - 1,
					std::stoi(vertexIndexInfo[2].substr(0, vertexIndexInfo[2].find('/'))) - 1
					});
			}
		}

	}

	const Vector3d& GetVertexByIndex(int index) const {
		return mVertexBuffer[index];
	}

	std::vector<Vector3d> mVertexBuffer;
	std::vector<Face> mFaceBuffer;
};



