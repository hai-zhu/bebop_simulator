clear; clc; close all;
%% training 
file_name = "yaw_trn";
bag = rosbag([file_name + ".bag"]);

bSel = select(bag,'Topic','/data_logger/cmd_vel');
cmd = readMessages(bSel,'DataFormat','struct');

% input data
u = [];
t = [];
for i = 1:numel(cmd)
    u = [u;cmd{i, 1}.Twist.Angular.Z]; % 90 deg/s
    timeStamp = double(cmd{i, 1}.Header.Stamp.Sec) + double(cmd{i, 1}.Header.Stamp.Nsec) * 1e-9;
    t = [t;timeStamp];
end

bSel = select(bag,'Topic','/simulator/odometry');
odom = readMessages(bSel,'DataFormat','struct');

% output data
y = [];
u_idx = 1;
for i = 1:numel(odom)
    timeStamp_next = double(odom{i, 1}.Header.Stamp.Sec) + double(odom{i, 1}.Header.Stamp.Nsec) * 1e-9;
    if(timeStamp_next) >= t(u_idx)
        quat = [odom{i-1, 1}.Pose.Pose.Orientation.W,...
                odom{i-1, 1}.Pose.Pose.Orientation.X,...
                odom{i-1, 1}.Pose.Pose.Orientation.Y,...
                odom{i-1, 1}.Pose.Pose.Orientation.Z];
        eul = quat2eul(quat);
        ang_last = eul(1) / pi * 180;
        quat = [odom{i, 1}.Pose.Pose.Orientation.W,...
                odom{i, 1}.Pose.Pose.Orientation.X,...
                odom{i, 1}.Pose.Pose.Orientation.Y,...
                odom{i, 1}.Pose.Pose.Orientation.Z];
        eul = quat2eul(quat);
        ang_next = eul(1) / pi * 180;
        timeStamp_last = double(odom{i-1, 1}.Header.Stamp.Sec) + double(odom{i-1, 1}.Header.Stamp.Nsec) * 1e-9;
        yawr = (ang_next - ang_last) / (timeStamp_next - timeStamp_last);
        y = [y;yawr];
        u_idx = u_idx + 1;
        if(u_idx) > numel(cmd)
           break; 
        end
    end
end
y = smooth(y);
Ts = mean(t(2:end) - t(1:end-1));
datatrn = iddata(y,u,Ts);
sys = n4sid(datatrn,2,'InputDelay',3);
%% testing 
file_name = "yaw_tst";
bag = rosbag([file_name + ".bag"]);

bSel = select(bag,'Topic','/data_logger/cmd_vel');
cmd = readMessages(bSel,'DataFormat','struct');

% input data
u = [];
t = [];
for i = 1:numel(cmd)
    u = [u;cmd{i, 1}.Twist.Angular.Z]; % 90 deg/s
    timeStamp = double(cmd{i, 1}.Header.Stamp.Sec) + double(cmd{i, 1}.Header.Stamp.Nsec) * 1e-9;
    t = [t;timeStamp];
end

bSel = select(bag,'Topic','/simulator/odometry');
odom = readMessages(bSel,'DataFormat','struct');

% output data
y = [];
u_idx = 1;
for i = 1:numel(odom)
    timeStamp_next = double(odom{i, 1}.Header.Stamp.Sec) + double(odom{i, 1}.Header.Stamp.Nsec) * 1e-9;
    if(timeStamp_next) >= t(u_idx)
        quat = [odom{i-1, 1}.Pose.Pose.Orientation.W,...
                odom{i-1, 1}.Pose.Pose.Orientation.X,...
                odom{i-1, 1}.Pose.Pose.Orientation.Y,...
                odom{i-1, 1}.Pose.Pose.Orientation.Z];
        eul = quat2eul(quat);
        ang_last = eul(1) / pi * 180;
        quat = [odom{i, 1}.Pose.Pose.Orientation.W,...
                odom{i, 1}.Pose.Pose.Orientation.X,...
                odom{i, 1}.Pose.Pose.Orientation.Y,...
                odom{i, 1}.Pose.Pose.Orientation.Z];
        eul = quat2eul(quat);
        ang_next = eul(1) / pi * 180;
        timeStamp_last = double(odom{i-1, 1}.Header.Stamp.Sec) + double(odom{i-1, 1}.Header.Stamp.Nsec) * 1e-9;
        yawr = (ang_next - ang_last) / (timeStamp_next - timeStamp_last);
        y = [y;yawr];
        u_idx = u_idx + 1;
        if(u_idx) > numel(cmd)
           break; 
        end
    end
end
y = smooth(y);
datatst = iddata(y,u,Ts);
sys.K = [0;0];
compare(datatst,sys);
sys