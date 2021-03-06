/******************** 声明 ************************/
/*  
    这是东北林业大学ARES机器人战队在Robomaster2018赛季关于计算机视觉部分
的完整代码，包含大小能量机关和视觉辅助瞄准的全部内容，应用在步兵机器人上。
（注：英雄机器人与哨兵机器人参考视觉辅助部分的代码，它们与步兵机器人在
 这一块是共用的）。虽然我们的成果与一些强队相比还有很大的差距，但是我们
 希望在某些技术点上我们的解决思路能给其他参赛队伍（特别是新参赛队伍）提供
 一定的借鉴，哪怕只有一点，那么我们的工作也是值得的。最后希望我们的成果能
 帮助到大家，祝愿Robomaster越办越红火！--- 来自一个为了机器人单身26年又即将
                                           离开校园的老人！
*/

//时间：2018.09.02
//地点：东北林业大学成栋楼大学生科技创新实验室
//作者：刘祖基、莫冲、侯弘毅、赖建伟、李士尧等

#include<opencv2/opencv.hpp> 
#include "Armor_Detector.h"
#include"bigSymbol.h"
#include "SerialPort.h" 

using namespace cv;
using namespace std;


//可选择模式
enum myModel
{
	sleep_model,
	auto_shoot_fire,
	big_symbol_fire
};
myModel Model;

//串口数据分析
CSerialPort com;
bool flagExposure = false;
void SPortReciveAnalyze(char * Rbuff, Armor_Detector& armor, sbol& symbol, bool& flag);


//读取摄像头数据
int McamaraCnt = 0;//视觉辅助摄像头读取计数
int ScamaraCnt = 0;//能量机关摄像头读取计数
bool camaraRead(myModel model, Armor_Detector& arm, sbol& sym);

int main()
{
 namedWindow("momoDebug:");
 //视觉辅助初始化
 Armor_Detector armor; //在构造函数中完成初始化
 if (!armor.camaraInit(0)) //摄像头初始化
 {
	 return 0;
 }
 //能量机关初始化
 sbol mySbol; //在构造函数中完成初始化
 if (!mySbol.camaraInit(1))
 {
	 return 0;
 }
 //串口初始化
 com.my_InitPort(3); //打开并配置串口
 waitKey(10);
 com.my_OpenListenThread(); //开启并运行串口监听线程
 waitKey(10);
 Model = sleep_model; // 默认为空闲模式
 unsigned int frames = 0;//计算帧率
 double time = 0;//计算帧率
 //红蓝方选择
 armor.ourteam = TeamRed;
 //armor.ourteam = TeamBlue;

 while (1)
 {
	 
	 double t0 = getTickCount();
	 frames++;
	 //串口接收数据解析
	SPortReciveAnalyze(com.my_recieve_data, armor, mySbol, flagExposure);
	 //摄像头采集图像
	if (!camaraRead(Model, armor, mySbol))
		 continue;
	 if (Model == auto_shoot_fire) //视觉辅助模式
	 {
		armor.autoShoot();
		
		 char cmd = 'c';
		 *(INT16*)&com.my_send_data[0] = -armor.ang_Y;
		 *(INT16*)&com.my_send_data[2] = -armor.ang_P;
		 *(INT16*)&com.my_send_data[4] = armor.AutoShoot;
		 if (com.UART_Send_Buff(cmd, com.my_send_data, 6))//串口发送
		 {
			 printf("成功发送 ===cmd:%c ===ang_Y:%d ===ang_P:%d \n", cmd, *(INT16*)&com.my_send_data[0],*(INT16*)&com.my_send_data[2]);
			 cout << "autoshoot" << armor.AutoShoot << endl;
		 }
	 }
	 else if(Model == big_symbol_fire)//能量机关模式
	 {
		 mySbol.BigPower();
		 char cmd = 'd';
		 *(INT16*)&com.my_send_data[0] = (INT16)(mySbol.angleY * 100);
		 *(INT16*)&com.my_send_data[2] = -(INT16)(mySbol.angleP * 100);
		 *(INT16*)&com.my_send_data[4] = (INT16)(mySbol.shootCnt);
		 *(float*)&com.my_send_data[6] = (float)(mySbol.Depth);
		 *(INT16*)&com.my_send_data[10] = (INT16)(mySbol.meanGrayValue);
		 if (com.UART_Send_Buff(cmd, com.my_send_data, 12)) //串口发送
		 {
			 printf("成功发送 ===cmd:%c ===ang_Y:%d ===ang_P:%d ===shootCnt:%d\n", cmd, (INT16)(mySbol.angleY * 100), -(INT16)(mySbol.angleP * 100), mySbol.shootCnt);
		 }
	 }
	 else if (Model == sleep_model) //空闲模式
	 {
		 Mat debugShow(128, 128, CV_8UC3, Scalar(0));
		 imshow("momoDebug:", debugShow);
		 waitKey(30);
		 cout << "************* sleep_model *************" << endl;
	 }
	

	 //测试帧率
	 time += (getTickCount() - t0) / getTickFrequency();
	 cout << frames / time << " fps" << endl;
	 if (frames / time < 10)//帧率小于10帧杀死当前程序重新启动
	 {
		 com.CloseListenTread();
		 com.my_Close_Port();
		 destroyAllWindows();
		 armor.capture_m.release();
		 mySbol.Capture_BigPower.release();
		 system("C:\\Users\\ares4\\Desktop\\restsrtar\\rest.bat");
		 break;
	 }
	 char c = waitKey(10);
	
	 //调节云台矫正角度
	 if (c == '1')
	 {
		 mySbol.corectAngY -= 0.2;
	 }
	 if (c == '2')
	 {
		 mySbol.corectAngY += 0.2;
	 }
	 if (c == '3')
	 {
		 mySbol.corectAngP -= 0.2;
	 }
	 if (c == '4')
	 {
		 mySbol.corectAngP += 0.2;
	 }
	
	 if (c == 'q')
		 break;
 }
 return 0;
}

//串口接收数据解析
void SPortReciveAnalyze(char * Rbuff, Armor_Detector& armor, sbol& symbol, bool& flag)
{
	static int cnt = 0;
	switch (Rbuff[1])
	{
	case 'c':
	{
		Model = auto_shoot_fire; 
		break;
	}
	case 'd':
	{
		Model = big_symbol_fire;
		if (Rbuff[4] == 's')
		{
			symbol.SymbolModel = numArab;
		}
		else if (Rbuff[4] == 'b')
		{
			symbol.SymbolModel = numFire;
		}
		break;
	}
	case 'z':
	{
		Model = sleep_model;
		symbol.hitCnt = 0;
		symbol.shootCnt1 = 0;
		//调节摄像头曝光
		if ((Rbuff[4] == 'u') && (flag == false))
		{
			int count = symbol.Capture_BigPower.get(CV_CAP_PROP_EXPOSURE);
			symbol.Capture_BigPower.set(CV_CAP_PROP_EXPOSURE, count + 1);
			cout << "++++++++++++++++++++++++++++++++EXPOSURE: " << count+1 << endl;
			flag = true;
		}
		else if ((Rbuff[4] == 'd') && (flag == false))
		{
			int count = symbol.Capture_BigPower.get(CV_CAP_PROP_EXPOSURE);
			symbol.Capture_BigPower.set(CV_CAP_PROP_EXPOSURE, count - 1);
			cout << "++++++++++++++++++++++++++++++++EXPOSURE: " << count-1 << endl;
			flag = true;
		}
		if (flag == true)
		{
			cnt++;
			if (cnt > 15)
			{
				cnt = 0;
				flag = false;
			}
		}
		break;
	}
	default:
		break;
	}
}

//读取摄像头数据
bool camaraRead(myModel model, Armor_Detector& arm, sbol& sym)
{
	bool flag = true;
	if (model == auto_shoot_fire)
	{
		arm.capture_m.read(arm.srcimage);
		if (!arm.srcimage.data)
		{
			cout << "视觉辅助摄像头没有读取到数据！" << endl;
			flag = false;
			return flag;
		}
		if (McamaraCnt < 5)
		{
			ScamaraCnt = 0;
			McamaraCnt++;
			flag = false;
			return flag;
		}
	}
	else if (model == big_symbol_fire)
	{
		sym.Capture_BigPower.read(sym.sbolImage);
		if (!sym.sbolImage.data)
		{
			cout << "大符摄像头没有读取到数据！" << endl;
			flag = false;
			return flag;
		}
		if (ScamaraCnt < 5)
		{
			McamaraCnt=0;
			ScamaraCnt ++;
			flag = false;
			return flag;
		}
	}
	return flag;
}

