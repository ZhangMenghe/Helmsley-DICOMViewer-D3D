#ifndef CVMATH_UTILS_H
#define CVMATH_UTILS_H

#include <glm/glm.hpp>
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

#endif // !MATH_UTILS_H
