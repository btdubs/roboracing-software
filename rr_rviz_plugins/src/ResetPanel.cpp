#include <pluginlib/class_list_macros.h>
#include <rr_rviz_plugins/ResetPanel.h>

#include <QVBoxLayout>

namespace rr_rviz_plugins {

ResetPanel::ResetPanel(QWidget *parent)
      : rviz::Panel(parent)  // Base class constructor
{
    reset_btn = new QPushButton("Reset!");
    reset_pub = nh.advertise<rr_msgs::race_reset>("/reset_detected", 0);
    connect(reset_btn, SIGNAL(released()), this, SLOT(resetCallback()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(reset_btn);
    setLayout(layout);
}

void ResetPanel::resetCallback() {
    rr_msgs::race_reset reset;
    reset_pub.publish(reset);
}

}  // namespace rr_rviz_plugins

PLUGINLIB_EXPORT_CLASS(rr_rviz_plugins::ResetPanel, rviz::Panel)
