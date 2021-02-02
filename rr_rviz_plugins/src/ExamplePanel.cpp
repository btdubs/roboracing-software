#include <pluginlib/class_list_macros.h>
#include <rr_rviz_plugins/ExamplePanel.h>

#include <QVBoxLayout>

/*
 * Just as in the header, everything needs to happen in the rr_rviz_plugins
 * namespace.
 */
namespace rr_rviz_plugins {

ExamplePanel::ExamplePanel(QWidget *parent)
      : rviz::Panel(parent)  // Base class constructor
{
    // Panels are allowed to interact with NodeHandles directly just like ROS
    // nodes.
    ros::NodeHandle handle;

    // Initialize a label for displaying some data
    QLabel *label = new QLabel("0 m/s");

    /* Initialize our subscriber to listen to the /speed topic.
     * Note the use of boost::bind, which allows us to give the callback a pointer to our UI label.
     */
    speed_subscriber =
          handle.subscribe<rr_msgs::speed>("/speed", 1, boost::bind(&ExamplePanel::speed_callback, this, _1, label));

    /* Use QT layouts to add widgets to the panel.
     * Here, we're using a VBox layout, which allows us to stack all of our
     * widgets vertically.
     */
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    setLayout(layout);
}

void ExamplePanel::speed_callback(const rr_msgs::speedConstPtr &msg, QLabel *label) {
    // Create the new contents of the label based on the speed message.
    auto text = std::to_string(msg->speed) + " m/s";
    // Set the contents of the label.
    label->setText(text.c_str());
}

// void ExamplePanel::paintEvent(QPaintEvent *event)  {
//
//}

}  // namespace rr_rviz_plugins

/*
 * IMPORTANT! This macro must be filled out correctly for your panel class.
 */
PLUGINLIB_EXPORT_CLASS(rr_rviz_plugins::ExamplePanel, rviz::Panel)
