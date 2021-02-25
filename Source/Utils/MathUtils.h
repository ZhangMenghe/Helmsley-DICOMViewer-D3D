#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <D3DPipeline/Primitive.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>

inline glm::quat getQuaternion(cv::Vec3d& rodrigues1x3) {
  double Q[4];
  cv::Mat R;
  cv::Rodrigues(rodrigues1x3, R);
  double trace = R.at<double>(0, 0) + R.at<double>(1, 1) + R.at<double>(2, 2);

  if (trace > 0.0)
  {
    double s = sqrt(trace + 1.0);
    Q[3] = (s * 0.5);
    s = 0.5 / s;
    Q[0] = ((R.at<double>(2, 1) - R.at<double>(1, 2)) * s);
    Q[1] = ((R.at<double>(0, 2) - R.at<double>(2, 0)) * s);
    Q[2] = ((R.at<double>(1, 0) - R.at<double>(0, 1)) * s);
  }

  else
  {
    int i = R.at<double>(0, 0) < R.at<double>(1, 1) ? (R.at<double>(1, 1) < R.at<double>(2, 2) ? 2 : 1) : (R.at<double>(0, 0) < R.at<double>(2, 2) ? 2 : 0);
    int j = (i + 1) % 3;
    int k = (i + 2) % 3;

    double s = sqrt(R.at<double>(i, i) - R.at<double>(j, j) - R.at<double>(k, k) + 1.0);
    Q[i] = s * 0.5;
    s = 0.5 / s;

    Q[3] = (R.at<double>(k, j) - R.at<double>(j, k)) * s;
    Q[j] = (R.at<double>(j, i) + R.at<double>(i, j)) * s;
    Q[k] = (R.at<double>(k, i) + R.at<double>(i, k)) * s;
  }
  return glm::quat(Q[3], Q[0], Q[1], Q[2]);
}

inline void getScreenToClientPos(float& x, float& y, float sw, float sh) {
    x = (2.0 * x - sw * 0.5f) / sw;
    y = 2.0f * (sh * 0.5f - y) / sh;
}
inline float shortest_distance(float x1, float y1,
    float z1, float a,
    float b, float c,
    float d) {
    d = fabs((a * x1 + b * y1 +
        c * z1 + d));
    float e = sqrt(a * a + b *
        b + c * c);
    return d / e;
}
inline glm::vec3 vec3MatNorm(glm::mat4 lmat, glm::vec3 v) {
    return glm::normalize(glm::vec3(lmat * glm::vec4(v, 1.0f)));
}

inline glm::mat4 rotMatFromDir(glm::vec3 dir) {
    return glm::toMat4(glm::rotation(dir, glm::vec3(.0, .0, -1.0f)));

    // glm::vec3 rotationZ = dir;
    // glm::vec3 rotationX = glm::normalize( glm::cross( glm::vec3( 0, 1, 0 ), rotationZ ) );
    // glm::vec3 rotationY = glm::normalize( glm::cross( rotationZ, rotationX ) );
    // glm::mat4 rotmat( rotationX.x, rotationY.x, rotationZ.x,  .0f,
    //                   rotationX.y, rotationY.y, rotationZ.y,  .0f,
    //                   rotationX.z, rotationY.z, rotationZ.z,  .0f,
    //                   .0f,         .0f,         .0f,  1.0f);
    // return rotmat;
}

//check 8 vertices of a standard cube with extend
inline glm::vec3 cloestVertexToPos(glm::vec3 pos, float extend) {
    return glm::vec3(0.5f);
}
inline glm::vec3 cloestVertexToPlane(const float* cuboid_with_texture, glm::vec3 pn, glm::vec3 p) {
    float d = -(pn.x * p.x + pn.y * p.y + pn.z * p.z);
    std::vector<int> vertex_idx;
    float shortest_dist = FLT_MAX;
    for (int i = 0; i < 8; i++) {
        float dist = shortest_distance(cuboid_with_texture[6 * i], cuboid_with_texture[6 * i + 1], cuboid_with_texture[6 * i + 2], pn.x, pn.y, pn.z, d);
        if (dist < shortest_dist) {
            shortest_dist = dist;
            vertex_idx.clear();
            vertex_idx.push_back(i);
        }
        else if (dist == shortest_dist) {
            vertex_idx.push_back(i);
        }
    }
    glm::vec3 res = glm::vec3(.0f);
    for (int id : vertex_idx)
        res += glm::vec3(cuboid_with_texture[6 * id], cuboid_with_texture[6 * id + 1], cuboid_with_texture[6 * id + 2]);

    return res * (1.0f / vertex_idx.size());
}

inline glm::vec3 rotateNormal(glm::mat4 modelMat, glm::vec3 ori_n) {
    return vec3MatNorm(glm::transpose(glm::inverse(modelMat)), ori_n);
}
inline glm::mat4 mouseRotateMat(glm::mat4 bmat, float xoffset, float yoffset) {
    return glm::rotate(glm::mat4(1.0f), xoffset, glm::vec3(0, 1, 0))
        * glm::rotate(glm::mat4(1.0f), yoffset, glm::vec3(1, 0, 0))
        * bmat;
}

#endif // !MATH_UTILS_H
