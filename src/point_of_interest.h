#ifndef POINT_OF_INTEREST_H
#define POINT_OF_INTEREST_H
#include <string>
#include <vector>
/**
 * The format of POI file should be a csv file.
 * It must contains a header, with the specified name of columns.
 * benchmark, file, linum, type
 */
class PointOfInterest {
public:
  PointOfInterest(std::string folder, std::string file, int linum, std::string type);
  ~PointOfInterest() {}

  std::string GetFilename();
  std::string GetPath();

  int GetLinum() {
    return m_linum;
  }
  std::string GetType() {
    return m_type;
  }
private:
  std::string m_folder;
  std::string m_file;
  int m_linum;
  std::string m_type;
};

std::vector<PointOfInterest*> create_point_of_interest(std::string folder, std::string poi_file);

#endif /* POINT_OF_INTEREST_H */