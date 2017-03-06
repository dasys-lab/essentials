#include "cnc_geometry/CNRobotAllo.h"

namespace geometry
{

CNRobotAllo::CNRobotAllo()
    : CNPositionAllo()
    , velocity()
{
    this->radius = 0;
    this->id = 0;
    this->certainty = 0;
    this->rotationVel = 0;
    this->opposer = std::make_shared<std::vector<int>>();
    this->supporter = std::make_shared<std::vector<int>>();
}

CNRobotAllo::~CNRobotAllo()
{
}

/**
 * Creates a string representation of this robot.
 * @return the string representing the robot.
 */
std::string CNRobotAllo::toString() const
{
    std::stringstream ss;
    ss << "CNRobot: ID: " << this->id << " Pose X: " << this->x << " Y: " << this->y << " Orientation: " << this->theta
       << " Velocity X: " << this->velocity.x << " Y: " << this->velocity.y << std::endl;
    return ss.str();
}

} /* namespace geometry */
