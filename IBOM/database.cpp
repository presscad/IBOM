//<LOCALE_TRANSLATE BY wbt />
#include "stdafx.h"
#include "PartLib.h"
#include "database.h"
#include "XhCharString.h"
#include "PartLibraryEx.h"

//1.接头为单剪连接时,采用外包角钢,外包角钢的宽度应比被连接角钢肢宽大一级
//  (长细比在80以下时,外包角钢肢厚再大一级),被连接角钢规格不同时,应取其小的规格
//2.接头为双剪连接时,采用内包角钢外贴板,内包角钢和外贴板的面积和宜不小于被连接角钢面积的1.3倍
//3.L140以上规格的角钢宜采用双包连接

//int g_nJgJointRecordNum=43;
//主材宽、厚 内包角钢宽、厚 外包钢板宽、厚 外包角钢宽、厚
/*JG_JOINT_PARA jg_joint_table[200] =
{
	{100, 7, 90, 7, 90, 8,110, 7},
	{100, 8, 90, 8, 90, 8,110, 8},
	{100,10, 90,10, 90,10,110,10},
	{100,12, 90,12, 90,12,110,12},
	{100,14, 90,14, 85,14,110,14},
	{100,16, 90,16, 80,16,110,16},
	{110, 7,100, 7,100, 8,125, 8},
	{110, 8,100, 8,100, 8,125, 8},
	{110,10,100,10,100,10,125,10},
	{110,12,100,12,100,12,125,12},
	{125, 8,110, 8,105, 8,140, 8},
	{125,10,110, 8,105, 8,140,10},
	{125,12,110,10,105,10,140,12},
	{140,10,125, 8,115,10,160,10},
	{140,12,125,10,115,10,160,12},
	{140,14,125,12,115,10,160,14},
	{160,10,140,10,135,10,180,10},
	{160,12,140,10,135,10,180,12},
	{160,14,140,12,135,12,180,14},
	{160,16,140,14,135,12,180,16},
	{180,12,160,10,155,10,200,12},
	{180,14,160,12,155,10,200,14},
	{180,16,160,14,155,12,200,16},
	{180,18,160,14,155,14,200,18},
	{200,14,180,12,180,12,220,14},
	{200,16,180,14,180,12,220,16},
	{200,18,180,14,180,14,220,18},
	{200,20,180,16,180,16,220,20},
	{200,24,180,18,180,18,220,24},
	{220,16,200,14,180,14,250,16},
	{220,18,200,16,180,14,250,18},
	{220,20,200,16,180,16,250,20},
	{220,22,200,18,180,18,250,22},
	{220,24,200,20,180,20,250,24},
	{220,26,200,24,180,20,250,26},
	{250,18,220,16,200,16,250,18},
	{250,20,220,18,200,16,250,20},
	{250,24,220,20,200,20,250,24},
	{250,26,220,22,200,22,250,26},
	{250,28,220,24,200,24,250,28},
	{250,30,220,26,200,24,250,30},
	{250,32,220,26,200,26,250,32},
	{250,35,220,26,200,30,250,35},
};*/

double Q235A_STABLE_COEF[251]=
{
	1.000, 	1.000, 	1.000, 	1.000, 	0.999, 	0.999, 	0.998, 	0.998, 	0.997, 	0.996, 
	0.995, 	0.994, 	0.993, 	0.992, 	0.991, 	0.989, 	0.988, 	0.986, 	0.985, 	0.983, 
	0.981, 	0.979, 	0.977, 	0.976, 	0.974, 	0.972, 	0.970, 	0.968, 	0.966, 	0.964, 
	0.963, 	0.961, 	0.959, 	0.957, 	0.955, 	0.952, 	0.950, 	0.948, 	0.946, 	0.944, 
	0.941, 	0.939, 	0.937, 	0.934, 	0.932, 	0.929, 	0.927, 	0.924, 	0.921, 	0.919, 
	0.916, 	0.913, 	0.910, 	0.907, 	0.904, 	0.900, 	0.897, 	0.894, 	0.890, 	0.886, 
	0.883, 	0.879, 	0.875, 	0.871, 	0.867, 	0.863, 	0.858, 	0.854, 	0.849, 	0.844, 
	0.839, 	0.834, 	0.829, 	0.824, 	0.818, 	0.813, 	0.807, 	0.801, 	0.795, 	0.789, 
	0.783, 	0.776, 	0.770, 	0.763, 	0.757, 	0.750, 	0.743, 	0.736, 	0.728, 	0.721, 
	0.714, 	0.706, 	0.699, 	0.691, 	0.684, 	0.676, 	0.668, 	0.661, 	0.653, 	0.645, 
	0.638, 	0.630, 	0.622, 	0.615, 	0.607, 	0.600, 	0.592, 	0.585, 	0.577, 	0.570, 
	0.563, 	0.555, 	0.548, 	0.541, 	0.534, 	0.527, 	0.520, 	0.514, 	0.507, 	0.500, 
	0.494, 	0.488, 	0.481, 	0.475, 	0.469, 	0.463, 	0.457, 	0.451, 	0.445, 	0.440, 
	0.434, 	0.429, 	0.423, 	0.418, 	0.412, 	0.407, 	0.402, 	0.397, 	0.392, 	0.387, 
	0.383, 	0.378, 	0.373, 	0.369, 	0.364, 	0.360, 	0.356, 	0.351, 	0.347, 	0.343, 
	0.339, 	0.335, 	0.331, 	0.327, 	0.323, 	0.320, 	0.316, 	0.312, 	0.309, 	0.305, 
	0.302, 	0.298, 	0.295, 	0.292, 	0.289, 	0.285, 	0.282, 	0.279, 	0.276, 	0.273, 
	0.270, 	0.267, 	0.264, 	0.262, 	0.259, 	0.256, 	0.253, 	0.251, 	0.248, 	0.246, 
	0.243, 	0.241, 	0.238, 	0.236, 	0.233, 	0.231, 	0.229, 	0.226, 	0.224, 	0.222, 
	0.220, 	0.218, 	0.215, 	0.213, 	0.211, 	0.209, 	0.207, 	0.205, 	0.203, 	0.201, 
	0.199, 	0.198, 	0.196, 	0.194, 	0.192, 	0.190, 	0.189, 	0.187, 	0.185, 	0.183, 
	0.182, 	0.180, 	0.179, 	0.177, 	0.175, 	0.174, 	0.172, 	0.171, 	0.169, 	0.168, 
	0.166, 	0.165, 	0.164, 	0.162, 	0.161, 	0.159, 	0.158, 	0.157, 	0.155, 	0.154, 
	0.153, 	0.152, 	0.150, 	0.149, 	0.148, 	0.147, 	0.146, 	0.144, 	0.143, 	0.142, 
	0.141, 	0.140, 	0.139, 	0.138, 	0.136, 	0.135, 	0.134, 	0.133, 	0.132, 	0.131, 
	0.130,
};
double Q235B_STABLE_COEF[251]=
{
	1.000, 	1.000, 	1.000, 	0.999, 	0.999, 	0.998, 	0.997, 	0.996, 	0.995, 	0.994, 
	0.992, 	0.991, 	0.989, 	0.987, 	0.985, 	0.983, 	0.981, 	0.978, 	0.976, 	0.973, 
	0.970, 	0.967, 	0.963, 	0.960, 	0.957, 	0.953, 	0.950, 	0.946, 	0.943, 	0.939, 
	0.936, 	0.932, 	0.929, 	0.925, 	0.922, 	0.918, 	0.914, 	0.910, 	0.906, 	0.903, 
	0.899, 	0.895, 	0.891, 	0.887, 	0.882, 	0.878, 	0.874, 	0.870, 	0.865, 	0.861, 
	0.856, 	0.852, 	0.847, 	0.842, 	0.838, 	0.833, 	0.828, 	0.823, 	0.818, 	0.813, 
	0.807,	0.802, 	0.797, 	0.791, 	0.786, 	0.780, 	0.774, 	0.769, 	0.763, 	0.757, 
	0.751,	0.745, 	0.739, 	0.732, 	0.726, 	0.720, 	0.714, 	0.707, 	0.701, 	0.694, 
	0.688,	0.681, 	0.675, 	0.668, 	0.661, 	0.655, 	0.648, 	0.641, 	0.635, 	0.628, 
	0.621,	0.614, 	0.608, 	0.601, 	0.594, 	0.588, 	0.581, 	0.575, 	0.568, 	0.561, 
	0.555,	0.549, 	0.542, 	0.536, 	0.529, 	0.523, 	0.517, 	0.511, 	0.505, 	0.499, 
	0.493,	0.487, 	0.481, 	0.475, 	0.470, 	0.464, 	0.458, 	0.453, 	0.447, 	0.442, 
	0.437,	0.432, 	0.426, 	0.421, 	0.416, 	0.411, 	0.406, 	0.402, 	0.397, 	0.392, 
	0.387,	0.383, 	0.378, 	0.374, 	0.370, 	0.365, 	0.361, 	0.357, 	0.353, 	0.349, 
	0.345,	0.341, 	0.337, 	0.333, 	0.329, 	0.326, 	0.322, 	0.318, 	0.315, 	0.311, 
	0.308,	0.304, 	0.301, 	0.298, 	0.295, 	0.291, 	0.288, 	0.285, 	0.282, 	0.279, 
	0.276,	0.273, 	0.270, 	0.267, 	0.265, 	0.262, 	0.259, 	0.256, 	0.254, 	0.251, 
	0.249,	0.246, 	0.244, 	0.241, 	0.239, 	0.236, 	0.234, 	0.232, 	0.229, 	0.227, 
	0.225,	0.223, 	0.220, 	0.218, 	0.216, 	0.214, 	0.212, 	0.210, 	0.208, 	0.206, 
	0.204,	0.202, 	0.200, 	0.198, 	0.197, 	0.195, 	0.193, 	0.191, 	0.190, 	0.188, 
	0.186,	0.184, 	0.183, 	0.181, 	0.180, 	0.178, 	0.176, 	0.175, 	0.173, 	0.172, 
	0.170,	0.169, 	0.167, 	0.166, 	0.165, 	0.163, 	0.162, 	0.160, 	0.159, 	0.158, 
	0.156,	0.155, 	0.154, 	0.153, 	0.151, 	0.150, 	0.149, 	0.148, 	0.146, 	0.145, 
	0.144,	0.143, 	0.142, 	0.141, 	0.140, 	0.138, 	0.137, 	0.136, 	0.135, 	0.134, 
	0.133,	0.132, 	0.131, 	0.130, 	0.129, 	0.128, 	0.127, 	0.126, 	0.125, 	0.124, 
	0.123,
};
double Q345A_STABLE_COEF[251]=
{
	1.0000,  1.0000,  1.0000,  0.9990,  0.9990,  0.9980,  0.9970,  0.9970,  0.9960,  0.9940,  
	0.9930,  0.9920,  0.9900,  0.9880,  0.9860,  0.9840,  0.9820,  0.9800,  0.9780,  0.9750,  
	0.9730,  0.9710,  0.9690,  0.9670,  0.9640,  0.9620,  0.9600,  0.9570,  0.9550,  0.9520,  
	0.9500,  0.9470,  0.9440,  0.9410,  0.9390,  0.9360,  0.9330,  0.9300,  0.9270,  0.9230,  
	0.9200,  0.9170,  0.9130,  0.9090,  0.9060,  0.9020,  0.8980,  0.8940,  0.8890,  0.8850,  
	0.8810,  0.8760,  0.8710,  0.8660,  0.8610,  0.8550,  0.8500,  0.8440,  0.8380,  0.8320,  
	0.8250,  0.8190,  0.8120,  0.8050,  0.7980,  0.7910,  0.7830,  0.7750,  0.7670,  0.7590,  
	0.7510,  0.7420,  0.7340,  0.7250,  0.7160,  0.7070,  0.6980,  0.6890,  0.6800,  0.6710,  
	0.6610,  0.6520,  0.6430,  0.6330,  0.6240,  0.6150,  0.6060,  0.5960,  0.5870,  0.5780,  
	0.5700,  0.5610,  0.5520,  0.5430,  0.5350,  0.5270,  0.5180,  0.5100,  0.5020,  0.4940,  
	0.4870,  0.4790,  0.4710,  0.4640,  0.4570,  0.4500,  0.4430,  0.4360,  0.4290,  0.4230,  
	0.4160,  0.4100,  0.4040,  0.3980,  0.3920,  0.3860,  0.3800,  0.3740,  0.3690,  0.3630,  
	0.3580,  0.3530,  0.3480,  0.3430,  0.3380,  0.3330,  0.3280,  0.3240,  0.3190,  0.3150,  
	0.3100,  0.3060,  0.3020,  0.2980,  0.2940,  0.2900,  0.2860,  0.2820,  0.2780,  0.2750,  
	0.2710,  0.2680,  0.2640,  0.2610,  0.2570,  0.2540,  0.2510,  0.2480,  0.2450,  0.2420,  
	0.2390,  0.2360,  0.2330,  0.2300,  0.2270,  0.2240,  0.2220,  0.2190,  0.2170,  0.2140,  
	0.2120,  0.2090,  0.2070,  0.2040,  0.2020,  0.2000,  0.1970,  0.1950,  0.1930,  0.1910,  
	0.1890,  0.1870,  0.1840,  0.1820,  0.1800,  0.1790,  0.1770,  0.1750,  0.1730,  0.1710,  
	0.1690,  0.1670,  0.1660,  0.1640,  0.1620,  0.1610,  0.1590,  0.1570,  0.1560,  0.1540,  
	0.1530,  0.1510,  0.1500,  0.1480,  0.1470,  0.1450,  0.1440,  0.1420,  0.1410,  0.1400,  
	0.1380,  0.1370,  0.1360,  0.1340,  0.1330,  0.1320,  0.1310,  0.1290,  0.1280,  0.1270,  
	0.1260,  0.1250,  0.1240,  0.1230,  0.1210,  0.1200,  0.1190,  0.1180,  0.1170,  0.1160,  
	0.1150,  0.1140,  0.1130,  0.1120,  0.1110,  0.1100,  0.1090,  0.1080,  0.1070,  0.1060,  
	0.1060,  0.1050,  0.1040,  0.1030,  0.1020,  0.1010,  0.1000,  0.0996,  0.0988,  0.0980,  
	0.0972,  0.0964,  0.0957,  0.0949,  0.0942,  0.0934,  0.0970,  0.0919,  0.0912,  0.0905,  
	0.0898,
};
double Q345B_STABLE_COEF[251]=
{
	1.0000, 1.0000, 1.0000, 0.9990, 0.9980, 0.9970, 0.9960, 0.9950, 0.9930, 0.9910,  
	0.9890, 0.9870, 0.9840, 0.9810, 0.9780, 0.9750, 0.9720, 0.9680, 0.9640, 0.9600,  
	0.9560, 0.9520, 0.9480, 0.9430, 0.9390, 0.9350, 1.0000, 0.9260, 0.9220, 0.9170,  
	0.9130, 0.9080, 0.9030, 0.8990, 0.8940, 0.8890, 0.8840, 0.8790, 0.8740, 0.8690,  
	0.8630, 0.8580, 0.8520, 0.8470, 0.8410, 0.8350, 0.8290, 0.8230, 0.8170, 0.8110,  
	0.8040, 0.7980, 0.7910, 0.7840, 0.7780, 0.7710, 0.7640, 0.7560, 0.7490, 0.7420,  
	0.7340, 0.7270, 0.7190, 0.7110, 0.7040, 0.6960, 0.6880, 0.6800, 0.6720, 0.6640,  
	0.6560, 0.6480, 0.6400, 0.6320, 0.6230, 0.6150, 0.6070, 0.5990, 0.5910, 0.5830,  
	0.5750, 0.5670, 0.5590, 0.5510, 0.5440, 0.5360, 0.5280, 0.5210, 0.5130, 0.5060,  
	0.4990, 0.4910, 0.4840, 0.4770, 0.4700, 0.4630, 0.4570, 0.4500, 0.4430, 0.4370,  
	0.4310, 0.4240, 0.4180, 0.4120, 0.4060, 0.4000, 0.3950, 0.3890, 0.3840, 0.3780,  
	0.3730, 0.3670, 0.3620, 0.3570, 0.3520, 0.3470, 0.3430, 0.3380, 0.3330, 0.3290,  
	0.3240, 0.3200, 0.3150, 0.3110, 0.3070, 0.3030, 0.2990, 0.2950, 0.2910, 0.2870,  
	0.2830, 0.2800, 0.2760, 0.2730, 0.2690, 0.2660, 0.2620, 0.2590, 0.2560, 0.2530,  
	0.2490, 0.2460, 0.2430, 0.2400, 0.2370, 0.2350, 0.2320, 0.2290, 0.2260, 0.2240,  
	0.2210, 0.2180, 0.2160, 0.2130, 0.2110, 0.2080, 0.2060, 0.2040, 0.2010, 0.1990,  
	0.1970, 0.1950, 0.1930, 0.1900, 0.1880, 0.1860, 0.1840, 0.1820, 0.1800, 0.1780,  
	0.1760, 0.1750, 0.1730, 0.1710, 0.1690, 0.1670, 0.1660, 0.1640, 0.1620, 0.1610,  
	0.1590, 0.1570, 0.1560, 0.1540, 0.1530, 0.1510, 0.1500, 0.1480, 0.1470, 0.1450,  
	0.1440, 0.1420, 0.1410, 0.1400, 0.1380, 0.1370, 0.1360, 0.1350, 0.1330, 0.1320,  
	0.1310, 0.1300, 0.1280, 0.1270, 0.1260, 0.1250, 0.1240, 0.1230, 0.1220, 0.1200,  
	0.1190, 0.1180, 0.1170, 0.1160, 0.1150, 0.1140, 0.1130, 0.1120, 0.1110, 0.1100,  
	0.1090, 0.1080, 0.1080, 0.1070, 0.1060, 0.1050, 0.1040, 0.1030, 0.1020, 0.1010,  
	0.1010, 0.0998, 0.0990, 0.0982, 0.0974, 0.0966, 0.0959, 0.0951, 0.0943, 0.0936,  
	0.0929, 0.0921, 0.0914, 0.0907, 0.0900, 0.0893, 0.0886, 0.0879, 0.0873, 0.0866,  
	0.0859,
};
double Q390A_STABLE_COEF[251]=
{
	1.0000,  1.0000,  1.0000,  0.9990,  0.9990,  0.9980,  0.9970,  0.9960,  0.9950,  0.9940,  
	0.9920,  0.9900,  9.9890,  0.9870,  0.9850,  0.9820,  0.9800,  0.9780,  0.9750,  0.9730,  
	0.9710,  0.9680,  0.9660,  0.9630,  0.9610,  0.9580,  0.9560,  0.9530,  0.9500,  0.9470,  
	0.9450,  0.9420,  0.9390,  0.9360,  0.9320,  0.9290,  0.9260,  0.9220,  0.9190,  0.9150,  
	0.9110,  0.9070,  0.9030,  0.8990,  0.8950,  0.8900,  0.8850,  0.8810,  0.8760,  0.8700,  
	0.8650,  0.8600,  0.8540,  0.8480,  0.8420,  0.8350,  0.8280,  0.8220,  0.8140,  0.8070,  
	0.8000,  0.7920,  0.7840,  0.7750,  0.7670,  0.7580,  0.7490,  0.7400,  0.7310,  0.7220,  
	0.7120,  0.7030,  0.6930,  0.6830,  0.6740,  0.6640,  0.6540,  0.6440,  0.6340,  0.6240,  
	0.6140,  0.6040,  0.5950,  0.5850,  0.5760,  0.5660,  0.5570,  0.5480,  0.5390,  0.5300,  
	0.5210,  0.5120,  0.5040,  0.4950,  0.4870,  0.4790,  0.4710,  0.4630,  0.4550,  0.4480,  
	0.4410,  0.4330,  0.4260,  0.4190,  0.4130,  0.4060,  0.3990,  0.3930,  0.3870,  0.3810,  
	0.3750,  0.3690,  0.3630,  0.3570,  0.3520,  0.3470,  0.3410,  0.3360,  0.3310,  0.3260,  
	0.3210,  0.3160,  0.3120,  0.3070,  0.3030,  0.2980,  0.2940,  0.2900,  0.2860,  0.2820,  
	0.2780,  0.2740,  0.2700,  0.2660,  0.2630,  0.2590,  0.2560,  0.2520,  0.2490,  0.2450,  
	0.2420,  0.2390,  0.2360,  0.2330,  0.2300,  0.2270,  0.2240,  0.2210,  0.2180,  0.2150,  
	0.2130,  0.2100,  0.2080,  0.2050,  0.2030,  0.2000,  0.1980,  0.1950,  0.1930,  0.1910,  
	0.1880,  0.1860,  0.1840,  0.1820,  0.1800,  0.1780,  0.1760,  0.1740,  0.1720,  0.1700,  
	0.1680,  0.1660,  0.1640,  0.1620,  0.1610,  0.1590,  0.1570,  0.1550,  0.1540,  0.1520,  
	0.1500,  0.1490,  0.1470,  0.1460,  0.1440,  0.1430,  0.1410,  0.1400,  0.1380,  0.1370,  
	0.1360,  0.1340,  0.1330,  0.1320,  0.1300,  0.1290,  0.1280,  0.1270,  0.1260,  0.1240,  
	0.1230,  0.1220,  0.1210,  0.1190,  0.1180,  0.1170,  0.1160,  0.1150,  0.1140,  0.1130,  
	0.1120,  0.1110,  0.1100,  0.1090,  0.1080,  0.1070,  0.1060,  0.1050,  0.1040,  0.1030,  
	0.1020,  0.1010,  0.1000,  0.0996,  0.0987,  0.0970,  0.0962,  0.0954,  0.0954,  0.0946,  
	0.0938,  0.0930,  0.0922,  0.0914,  0.0907,  0.0899,  0.0892,  0.0884,  0.0877,  0.0870,  
	0.0863,  0.0856,  0.0849,  0.0842,  0.0836,  0.0829,  0.0823,  0.0816,  0.0810,  0.0803,  
	0.0797,
};
double Q390B_STABLE_COEF[251]=
{
	1.0000, 1.0000, 1.0000, 0.9990, 0.9980, 0.9970, 0.9960, 0.9940, 0.9920, 0.9900, 
	0.9880, 0.9850, 0.9820, 0.9790, 0.9760, 0.9720, 0.9680, 0.9640, 0.9590, 0.9550, 
	0.9510, 0.9460, 0.9420, 0.9370, 0.9330, 0.9280, 0.9230, 0.9190, 0.9140, 0.9090, 
	0.9040, 0.8990, 0.8940, 0.8890, 0.8830, 0.8780, 0.8720, 0.8670, 0.8610, 0.8550, 
	0.8490, 0.8430, 0.8370, 0.8310, 0.8240, 0.8180, 0.8110, 0.8040, 0.7970, 0.7900, 
	0.7830, 0.7760, 0.7690, 0.7610, 0.7530, 0.7460, 0.7380, 0.7300, 0.7220, 0.7140, 
	0.7050, 0.6970, 0.6890, 0.6800, 0.6720, 0.6630, 0.6540, 0.6460, 0.6370, 0.6290, 
	0.6200, 0.6110, 0.6030, 0.5940, 0.5860, 0.5770, 0.5690, 0.5600, 0.5520, 0.5440, 
	0.5350, 0.5270, 0.5190, 0.5110, 0.5040, 0.4960, 0.4880, 0.4810, 0.4730, 0.4660, 
	0.4590, 0.4520, 0.4450, 0.4380, 0.4310, 0.4240, 0.4180, 0.4110, 0.4050, 0.3990, 
	0.3930, 0.3870, 0.3810, 0.3750, 0.3700, 0.3640, 0.3590, 0.3530, 0.3480, 0.3430, 
	0.3380, 0.3330, 0.3280, 0.3230, 0.3190, 0.3140, 0.3100, 0.3050, 0.3010, 0.2970, 
	0.2930, 0.2890, 0.2850, 0.2810, 0.2770, 0.2730, 0.2690, 0.2660, 0.2620, 0.2590, 
	0.2550, 0.2520, 0.2480, 0.2450, 0.2420, 0.2390, 0.2360, 0.2330, 0.2300, 0.2270, 
	0.2240, 0.2210, 0.2180, 0.2160, 0.2130, 0.2100, 0.2080, 0.2050, 0.2030, 0.2000, 
	0.1980, 0.1960, 0.1930, 0.1910, 0.1890, 0.1870, 0.1840, 0.1820, 0.1800, 0.1780, 
	0.1760, 0.1740, 0.1720, 0.1700, 0.1680, 0.1670, 0.1650, 0.1630, 0.1610, 0.1590, 
	0.1580, 0.1560, 0.1540, 0.1530, 0.1510, 0.1500, 0.1480, 0.1460, 0.1450, 0.1430, 
	0.1420, 0.1400, 0.1390, 0.1380, 0.1360, 0.1350, 0.1340, 0.1320, 0.1310, 0.1300, 
	0.1280, 0.1270, 0.1260, 0.1250, 0.1230, 0.1220, 0.1210, 0.1200, 0.1190, 0.1180, 
	0.1170, 0.1160, 0.1140, 0.1130, 0.1120, 0.1110, 0.1100, 0.1090, 0.1080, 0.1070, 
	0.1060, 0.1050, 0.1050, 0.1040, 0.1030, 0.1020, 0.1010, 0.1000, 0.0992, 0.0983, 
	0.0975, 0.0967, 0.0958, 0.0950, 0.0942, 0.0934, 0.0926, 0.0919, 0.0911, 0.0904, 
	0.0896, 0.0889, 0.0882, 0.0874, 0.0867, 0.0860, 0.0853, 0.0847, 0.0840, 0.0833, 
	0.0827, 0.0820, 0.0814, 0.0807, 0.0801, 0.0795, 0.0789, 0.0783, 0.0777, 0.0771, 
	0.0765,									
};
double Q420A_STABLE_COEF[251]=
{
	1.000,1.000,1.000,0.999,0.999,0.998,0.997,0.996,0.995,0.993,
	0.992,0.990,0.988,0.986,0.983,0.981,0.978,0.976,0.974,0.971,
	0.969,0.966,0.964,0.961,0.958,0.956,0.953,0.950,0.947,0.944,
	0.941,0.938,0.935,0.932,0.928,0.925,0.921,0.917,0.913,0.909,
	0.905,0.901,0.896,0.892,0.887,0.882,0.877,0.872,0.866,0.860,
	0.854,0.848,0.842,0.835,0.828,0.821,0.814,0.806,0.798,0.790,
	0.782,0.773,0.764,0.755,0.746,0.736,0.727,0.717,0.707,0.697,
	0.687,0.677,0.666,0.656,0.646,0.636,0.625,0.615,0.605,0.595,
	0.585,0.575,0.565,0.556,0.546,0.537,0.527,0.518,0.509,0.501,
	0.492,0.483,0.475,0.467,0.459,0.451,0.443,0.436,0.428,0.421,
	0.414,0.407,0.400,0.394,0.387,0.381,0.375,0.369,0.363,0.357,
	0.351,0.346,0.340,0.335,0.329,0.324,0.319,0.314,0.310,0.305,
	0.300,0.296,0.291,0.287,0.283,0.279,0.275,0.271,0.267,0.263,
	0.259,0.256,0.252,0.249,0.245,0.242,0.239,0.235,0.232,0.229,
	0.226,0.223,0.220,0.217,0.214,0.212,0.209,0.206,0.204,0.201,
	0.198,0.196,0.194,0.191,0.189,0.186,0.184,0.182,0.180,0.178,
	0.176,0.174,0.171,0.169,0.168,0.166,0.164,0.162,0.160,0.158,
	0.156,0.155,0.153,0.151,0.150,0.148,0.146,0.145,0.143,0.142,
	0.140,0.139,0.137,0.136,0.134,0.133,0.132,0.130,0.129,0.128,
	0.126,0.125,0.124,0.123,0.121,0.120,0.119,0.118,0.117,0.116,
	0.114,0.113,0.112,0.111,0.110,0.109,0.108,0.107,0.106,0.105,
	0.104,0.103,0.102,0.101,0.100,0.099,0.099,0.098,0.097,0.096,
	0.095,0.094,0.093,0.093,0.092,0.091,0.090,0.089,0.089,0.088,
	0.087,0.087,0.086,0.085,0.084,0.084,0.083,0.082,0.082,0.081,
	0.080,0.080,0.079,0.078,0.078,0.077,0.077,0.076,0.075,0.075,
	0.074,
};
double Q420B_STABLE_COEF[251]=
{
	1.000,1.000,0.999,0.999,0.998,0.997,0.995,0.993,0.991,0.989,
	0.987,0.984,0.981,0.977,0.974,0.970,0.965,0.961,0.956,0.952,
	0.947,0.943,0.938,0.933,0.929,0.924,0.919,0.914,0.909,0.903,
	0.898,0.893,0.887,0.882,0.876,0.871,0.865,0.859,0.853,0.846,
	0.840,0.834,0.827,0.820,0.813,0.806,0.799,0.792,0.785,0.777,
	0.769,0.762,0.754,0.746,0.737,0.729,0.721,0.712,0.704,0.695,
	0.686,0.678,0.669,0.660,0.651,0.642,0.633,0.624,0.615,0.606,
	0.597,0.588,0.579,0.571,0.562,0.553,0.545,0.536,0.528,0.519,
	0.511,0.503,0.495,0.487,0.479,0.472,0.464,0.457,0.449,0.442,
	0.435,0.428,0.421,0.415,0.408,0.402,0.395,0.389,0.383,0.377,
	0.371,0.365,0.360,0.354,0.349,0.343,0.338,0.333,0.328,0.323,
	0.318,0.313,0.309,0.304,0.300,0.295,0.291,0.287,0.283,0.279,
	0.275,0.271,0.267,0.263,0.260,0.256,0.253,0.249,0.246,0.242,
	0.239,0.236,0.233,0.230,0.227,0.224,0.221,0.218,0.215,0.212,
	0.210,0.207,0.204,0.202,0.199,0.197,0.194,0.192,0.190,0.187,
	0.185,0.183,0.181,0.179,0.177,0.175,0.172,0.170,0.169,0.167,
	0.165,0.163,0.161,0.159,0.157,0.156,0.154,0.152,0.151,0.149,
	0.147,0.146,0.144,0.143,0.141,0.140,0.138,0.137,0.135,0.134,
	0.132,0.131,0.130,0.128,0.127,0.126,0.125,0.123,0.122,0.121,
	0.120,0.119,0.117,0.116,0.115,0.114,0.113,0.112,0.111,0.110,
	0.109,0.108,0.107,0.106,0.105,0.104,0.103,0.102,0.101,0.100,
	0.099,0.098,0.097,0.097,0.096,0.095,0.094,0.093,0.092,0.092,
	0.091,0.090,0.089,0.089,0.088,0.087,0.086,0.086,0.085,0.084,
	0.084,0.083,0.082,0.081,0.081,0.080,0.080,0.079,0.078,0.078,
	0.077,0.076,0.076,0.075,0.075,0.074,0.073,0.073,0.072,0.072,
	0.071,
};
double Q460A_STABLE_COEF[251]=
{
	1.000,1.000,1.000,0.999,0.999,0.998,0.997,0.995,0.994,0.992,
	0.991,0.989,0.987,0.984,0.982,0.979,0.977,0.974,0.972,0.969,
	0.966,0.964,0.961,0.958,0.955,0.952,0.950,0.946,0.943,0.940,
	0.937,0.933,0.930,0.926,0.923,0.919,0.915,0.910,0.906,0.902,
	0.897,0.892,0.887,0.882,0.877,0.871,0.865,0.859,0.853,0.846,
	0.840,0.833,0.825,0.818,0.810,0.802,0.793,0.784,0.776,0.766,
	0.757,0.747,0.737,0.727,0.717,0.707,0.696,0.686,0.675,0.664,
	0.654,0.643,0.632,0.621,0.611,0.600,0.590,0.579,0.569,0.559,
	0.549,0.539,0.529,0.520,0.510,0.501,0.492,0.483,0.474,0.466,
	0.457,0.449,0.441,0.433,0.426,0.418,0.411,0.404,0.397,0.390,
	0.383,0.377,0.370,0.364,0.358,0.352,0.346,0.340,0.335,0.329,
	0.324,0.319,0.313,0.308,0.304,0.299,0.294,0.290,0.285,0.281,
	0.276,0.272,0.268,0.264,0.260,0.256,0.253,0.249,0.245,0.242,
	0.238,0.235,0.232,0.228,0.225,0.222,0.219,0.216,0.213,0.210,
	0.207,0.205,0.202,0.199,0.197,0.194,0.192,0.189,0.187,0.184,
	0.182,0.180,0.178,0.175,0.173,0.171,0.169,0.167,0.165,0.163,
	0.161,0.159,0.157,0.155,0.154,0.152,0.150,0.148,0.147,0.145,
	0.143,0.142,0.140,0.139,0.137,0.136,0.134,0.133,0.131,0.130,
	0.128,0.127,0.126,0.124,0.123,0.122,0.121,0.119,0.118,0.117,
	0.116,0.115,0.113,0.112,0.111,0.110,0.109,0.108,0.107,0.106,
	0.105,0.104,0.103,0.102,0.101,0.100,0.099,0.098,0.097,0.096,
	0.095,0.094,0.094,0.093,0.092,0.091,0.090,0.089,0.089,0.088,
	0.087,0.086,0.086,0.085,0.084,0.083,0.083,0.082,0.081,0.081,
	0.080,0.079,0.079,0.078,0.077,0.077,0.076,0.075,0.075,0.074,
	0.073,0.073,0.072,0.072,0.071,0.071,0.070,0.069,0.069,0.068,
	0.068,
};
double Q460B_STABLE_COEF[251]=
{
	1.000,1.000,0.999,0.999,0.998,0.996,0.995,0.993,0.991,0.988,
	0.985,0.982,0.979,0.975,0.971,0.967,0.962,0.957,0.953,0.948,
	0.943,0.938,0.933,0.928,0.923,0.918,0.913,0.907,0.902,0.896,
	0.891,0.885,0.879,0.873,0.867,0.861,0.855,0.848,0.842,0.835,
	0.828,0.821,0.814,0.806,0.799,0.791,0.784,0.776,0.768,0.759,
	0.751,0.743,0.734,0.725,0.716,0.708,0.699,0.689,0.680,0.671,
	0.662,0.652,0.643,0.634,0.624,0.615,0.605,0.596,0.587,0.578,
	0.568,0.559,0.550,0.541,0.532,0.524,0.515,0.506,0.498,0.490,
	0.482,0.473,0.466,0.458,0.450,0.442,0.435,0.428,0.421,0.414,
	0.407,0.400,0.393,0.387,0.381,0.374,0.368,0.362,0.356,0.351,
	0.345,0.340,0.334,0.329,0.324,0.319,0.314,0.309,0.304,0.299,
	0.295,0.290,0.286,0.282,0.277,0.273,0.269,0.265,0.262,0.258,
	0.254,0.250,0.247,0.243,0.240,0.237,0.233,0.230,0.227,0.224,
	0.221,0.218,0.215,0.212,0.209,0.206,0.204,0.201,0.198,0.196,
	0.193,0.191,0.188,0.186,0.184,0.181,0.179,0.177,0.175,0.173,
	0.171,0.168,0.166,0.164,0.163,0.161,0.159,0.157,0.155,0.153,
	0.151,0.150,0.148,0.146,0.145,0.143,0.142,0.140,0.138,0.137,
	0.135,0.134,0.132,0.131,0.130,0.128,0.127,0.126,0.124,0.123,
	0.122,0.120,0.119,0.118,0.117,0.116,0.114,0.113,0.112,0.111,
	0.110,0.109,0.108,0.107,0.106,0.105,0.104,0.103,0.102,0.101,
	0.100,0.099,0.098,0.097,0.096,0.095,0.094,0.094,0.093,0.092,
	0.091,0.090,0.089,0.089,0.088,0.087,0.086,0.086,0.085,0.084,
	0.083,0.083,0.082,0.081,0.081,0.080,0.079,0.079,0.078,0.077,
	0.077,0.076,0.075,0.075,0.074,0.074,0.073,0.072,0.072,0.071,
	0.071,0.070,0.070,0.069,0.068,0.068,0.067,0.067,0.066,0.066,
	0.065,
};
//CalStableCoef1()函数的原始计算公式，现暂时不用 wjh-2013.5.30
static double RawCalStableCoef(double fy,double lamda,char sectionType)
{
	double lamda_=lamda/Pi*sqrt(fy/206000);
	double lamda_2=lamda_*lamda_;
	double alfa1=0.41;
	double alfa2=0.986;
	double alfa3=0.152;
	if(sectionType!='A')
	{
		alfa1=0.65;
		alfa2=0.965;
		alfa3=0.3;
	}
	double fai;
	if(lamda_<=0.215)
		fai=1-alfa1*lamda_2;
	else
	{
		double expr=alfa2+alfa3*lamda_+lamda_2;
		fai=0.5*(expr-sqrt(expr*expr-4*lamda_2))/lamda_2;
	}
	return fai;
}
//中国规范
double CalStableCoef1(double lamda,char sectionType,char cMaterial)
{
	double fai=0;
	int index=min((int)lamda,249);
	double coef=lamda-index;
	if(sectionType=='A')
	{
		if(cMaterial==CSteelMatLibrary::Q235BriefMark())//'S')	//Q235
			fai=Q235A_STABLE_COEF[index]+coef*(Q235A_STABLE_COEF[index+1]-Q235A_STABLE_COEF[index]);
		else if(cMaterial==CSteelMatLibrary::Q345BriefMark())//'H')	//Q345	
			fai=Q345A_STABLE_COEF[index]+coef*(Q345A_STABLE_COEF[index+1]-Q345A_STABLE_COEF[index]);
		else if(cMaterial==CSteelMatLibrary::Q390BriefMark())//'G')	//Q390	
			fai=Q390A_STABLE_COEF[index]+coef*(Q390A_STABLE_COEF[index+1]-Q390A_STABLE_COEF[index]);
		else if(cMaterial==CSteelMatLibrary::Q420BriefMark())//'P')	//Q420	
			fai=Q420A_STABLE_COEF[index]+coef*(Q420A_STABLE_COEF[index+1]-Q420A_STABLE_COEF[index]);
		else if(cMaterial==CSteelMatLibrary::Q460BriefMark())//'T')	//Q460	
			fai=Q460A_STABLE_COEF[index]+coef*(Q460A_STABLE_COEF[index+1]-Q460A_STABLE_COEF[index]);
	}
	else if(sectionType=='B')
	{
		if(coef>1)
			coef=1;
		if(cMaterial==CSteelMatLibrary::Q235BriefMark())//'S')	//Q235
			fai=Q235B_STABLE_COEF[index]+coef*(Q235B_STABLE_COEF[index+1]-Q235B_STABLE_COEF[index]);
		else if(cMaterial==CSteelMatLibrary::Q345BriefMark())//'H')	//Q345
			fai=Q345B_STABLE_COEF[index]+coef*(Q345B_STABLE_COEF[index+1]-Q345B_STABLE_COEF[index]);
		else if(cMaterial==CSteelMatLibrary::Q390BriefMark())//'G')	//Q390
			fai=Q390B_STABLE_COEF[index]+coef*(Q390B_STABLE_COEF[index+1]-Q390B_STABLE_COEF[index]);
		else if(cMaterial==CSteelMatLibrary::Q420BriefMark())//'P')	//Q420
			fai=Q420B_STABLE_COEF[index]+coef*(Q420B_STABLE_COEF[index+1]-Q420B_STABLE_COEF[index]);
		else if(cMaterial==CSteelMatLibrary::Q460BriefMark())//'T')	//Q460
			fai=Q460B_STABLE_COEF[index]+coef*(Q460B_STABLE_COEF[index+1]-Q460B_STABLE_COEF[index]);
	}
	return fai;
}
//美国导则
double CalStableCoef2(double lamda,double E,double fy)
{
	//美国导则
	double fa,Cc;	//受压稳定强度，受压稳定长细比临界值
	Cc=Pi*sqrt(2*E/fy);
	if(lamda<=Cc)
		fa=(1-0.5*pow(lamda/Cc,2))*fy;
	else 
		fa=Pi*Pi*E/pow(lamda,2);
	return fa/fy;
}
double CalStableCoef(int iDesignCode,double lamda,STEELMAT *pMatType,int sectionTypeOrThick)
{
	if(iDesignCode!=2)	//国内规范
		return CalStableCoef1(lamda,(char)sectionTypeOrThick,pMatType->cBriefMark);
	else				//美国ASCE 10-97导则
		return CalStableCoef2(lamda,pMatType->E,pMatType->fy(sectionTypeOrThick));
}
BOOL JgZhunJuSerialize(CString sFileName, BOOL bWrite,BOOL bThrowError/*=TRUE*/)
{
	FILE *fp;
	char file_path[MAX_PATH];
	sprintf(file_path,"%s",sFileName);
	int iRow,iCol;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
			{
				if(bThrowError)
#ifdef AFX_TARG_ENU_ENGLISH
					throw "Can't open the stadia file of angle,the database remains unchanged.";
#else 
					throw "打不开角钢准距文件,准距数据库将保持不变";
#endif
				else
					return FALSE;
			}
			fprintf(fp,"%d \n",zhunju_N);
			for(iRow=0;iRow<zhunju_N;iRow++)
			{
				for(iCol=0;iCol<5;iCol++)
				{
					WORD* record_para=(WORD*)&zhunju_table[iRow];
					fprintf(fp,"%d ",record_para[iCol]);
				}
				fprintf(fp,"\n");
			}
		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
			{
				if(bThrowError)
#ifdef AFX_TARG_ENU_ENGLISH
					throw "Can't open the stadia database of angle,the database will be the default";
#else 
					throw "打不开角钢准距库,准距数据将缺省";
#endif
				else
					return FALSE;
			}
			fscanf(fp,"%d",&zhunju_N);
			for(iRow=0;iRow<zhunju_N;iRow++)
			{
				WORD* record_para=(WORD*)&zhunju_table[iRow];
				for(iCol=0;iCol<5;iCol++)
				{
					fscanf(fp,"%d",&record_para[iCol]);
					if(zhunju_N>100)
					{
						fclose(fp);
						if(bThrowError)
#ifdef AFX_TARG_ENU_ENGLISH
							throw "System can only handle 100 stadia of angle's specifications at the same time";
#else 
							throw "系统只能同时处理100种角钢规格的准距";
#endif
						else
							return FALSE;
					}
					if(iCol==0)
					{
						int wide=record_para[iCol];
						if( record_para[iCol]<40
							||record_para[iCol]>500)
						{
							fclose(fp);
							if(bThrowError)
#ifdef AFX_TARG_ENU_ENGLISH
								throw "Angle's stadia database is broken";
#else 
								throw "角钢准距库文件已被破坏";
#endif
							else
								return FALSE;
						}
					}
					else 
					{
						if(record_para[iCol]<0||record_para[iCol]>=record_para[0])
						{
							fclose(fp);
							if(bThrowError)
#ifdef AFX_TARG_ENU_ENGLISH
								throw "The stadia of data file is illegal, import failure";
#else 
								throw "数据文件中的准距不合法,导入失败";
#endif
							else
								return FALSE;
						}
					}
				}
			}
		}
		fclose(fp);
	}
	catch(CString sError)
	{
		if(bThrowError)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}
BOOL JgGuiGeSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt)
{
	FILE *fp;
	char file_path[MAX_PATH];
	sprintf(file_path,"%s",sFileName);
	if(strlen(file_path)<=0)
		return FALSE;
	int iRow;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw CString("Can't open angle's specifications database,the database remains unchanged");
#else 
				throw CString("打不开角钢规格库文件,角钢规格库保持不变");
#endif
			fprintf(fp,"%d \n",jgguige_N);
			for(iRow=0;iRow<jgguige_N;iRow++)
			{
				fprintf(fp,"%8.3f %8.3f %C %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f",jgguige_table[iRow].wing_wide,
					jgguige_table[iRow].wing_thick,jgguige_table[iRow].cType,jgguige_table[iRow].r,
					jgguige_table[iRow].area,jgguige_table[iRow].theroy_weight,jgguige_table[iRow].Ix,
					jgguige_table[iRow].Rx,jgguige_table[iRow].Wx,
					jgguige_table[iRow].Iy0,jgguige_table[iRow].Ry0,
					jgguige_table[iRow].Wy0);
				fprintf(fp,"\n");
			}
		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw CString("Can't open angle's specifications database,the database will be default");
#else 
				throw CString("打不开角钢规格库文件,角钢规格将缺省");
#endif
			fscanf(fp,"%d",&jgguige_N);
			if(jgguige_N>150)
			{
				fclose(fp);
#ifdef AFX_TARG_ENU_ENGLISH
				throw CString("The system can only handle 150 specifications of angle at the same time");
#else 
				throw CString("系统只能同时处理150种角钢规格");
#endif
			}
			for(iRow=0;iRow<jgguige_N;iRow++)
			{
				fscanf(fp,"%lf%lf%s%lf%lf%lf%lf%lf%lf%lf%lf%lf",&jgguige_table[iRow].wing_wide,
					&jgguige_table[iRow].wing_thick,&jgguige_table[iRow].cType,&jgguige_table[iRow].r,
					&jgguige_table[iRow].area,&jgguige_table[iRow].theroy_weight,&jgguige_table[iRow].Ix,
					&jgguige_table[iRow].Rx,&jgguige_table[iRow].Wx,
					&jgguige_table[iRow].Iy0,&jgguige_table[iRow].Ry0,
					&jgguige_table[iRow].Wy0);
				if( jgguige_table[iRow].wing_wide<40
					||jgguige_table[iRow].wing_wide>500)
				{
					fclose(fp);
#ifdef AFX_TARG_ENU_ENGLISH
					throw CString("The file is broken");
#else 
					throw CString("此文件已被破坏");
#endif
				}
			}
		}
		fclose(fp);
	}
	catch(CString sError)
	{
		if(bPrompt)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}

BOOL TubeGuiGeSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt)
{
	FILE *fp;
	char file_path[MAX_PATH],line_txt[200];
	sprintf(file_path,"%s",sFileName);
	if(strlen(file_path)<=0)
		return FALSE;
	int iRow;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open tube's specifications database, the database remains unchanged";
#else 
				throw "打不开钢管规格库文件,钢管规格库保持不变";
#endif
			bool bCodeMode=false;
			if(bCodeMode)
			{	//仅用于更析代码中数据段用
				fprintf(fp,"%d \n",tubeguige_N);
				CXhChar500 lineText;
				for(iRow=0;iRow<tubeguige_N;iRow++)
				{	
					lineText.Printf("	%s,%s,%s,%s,%s,%s,%s,",
						(char*)CXhChar16(tubeguige_table[iRow].D),
						(char*)CXhChar16(tubeguige_table[iRow].thick),
						(char*)CXhChar16(tubeguige_table[iRow].area),
						(char*)CXhChar16(tubeguige_table[iRow].theroy_weight),
						(char*)CXhChar16(tubeguige_table[iRow].I),
						(char*)CXhChar16(tubeguige_table[iRow].r),
						(char*)CXhChar16(tubeguige_table[iRow].W));
					lineText.Append("\n");
					fputs(lineText,fp);
				}
				fclose(fp);
				return TRUE;
			}
			fprintf(fp,"%d \n",tubeguige_N);
			for(iRow=0;iRow<tubeguige_N;iRow++)
			{
				sprintf(line_txt,"%.1f %lf %lf %lf %lf %lf %lf\n",tubeguige_table[iRow].D,
					tubeguige_table[iRow].thick,tubeguige_table[iRow].area,
					tubeguige_table[iRow].theroy_weight,tubeguige_table[iRow].I,
					tubeguige_table[iRow].r,tubeguige_table[iRow].W);
				fputs(line_txt,fp);
			}
		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open tube's specifications database,the database will be default";
#else 
				throw "打不开钢管规格库文件,钢管规格将缺省";
#endif
			fscanf(fp,"%d",&tubeguige_N);
			fgets(line_txt,200,fp);
			if(tubeguige_N>200)
			{
				fclose(fp);
#ifdef AFX_TARG_ENU_ENGLISH
				throw "The system can only handle 200 specifications of tube at the same time";
#else 
				throw "系统只能同时处理200种钢管规格";
#endif
			}
			for(iRow=0;iRow<tubeguige_N;iRow++)
			{
				fgets(line_txt,200,fp);
				sscanf(line_txt,"%lf %lf %lf %lf %lf %lf %lf",&tubeguige_table[iRow].D,
					&tubeguige_table[iRow].thick,&tubeguige_table[iRow].area,
					&tubeguige_table[iRow].theroy_weight,&tubeguige_table[iRow].I,
					&tubeguige_table[iRow].r,&tubeguige_table[iRow].W);
			}
		}
		fclose(fp);
	}
	catch(char* sError)
	{
		if(bPrompt)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}

BOOL FlatGuiGeSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt)
{
	FILE *fp;
	char file_path[MAX_PATH];
	sprintf(file_path,"%s",sFileName);
	if(strlen(file_path)<=0)
		return FALSE;
	int iRow;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw CString("Can't open  flat iron's specifications database,the database remain the same");
#else 
				throw CString("打不开扁铁规格库文件,扁铁规格库保持不变");
#endif
			fprintf(fp,"%d \n",bianguige_N);
			for(iRow=0;iRow<bianguige_N;iRow++)
			{
				fprintf(fp,"%8.3f %8.3f %8.3f",bianguige_table[iRow].wide,
					bianguige_table[iRow].thick,bianguige_table[iRow].theroy_weight);
				fprintf(fp,"\n");
			}
		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw CString("Can't open U-steel's specification database,the database will be default");
#else 
				throw CString("打不开扁铁规格库文件,扁铁规格将缺省");
#endif
			fscanf(fp,"%d",&bianguige_N);
			if(bianguige_N>100)
			{
				fclose(fp);
#ifdef AFX_TARG_ENU_ENGLISH
				throw CString("The system can only handle 100 specifications of flat iron at the same time");
#else 
				throw CString("系统只能同时处理100种扁铁规格");
#endif
			}
			for(iRow=0;iRow<bianguige_N;iRow++)
			{
				fscanf(fp,"%lf%lf%lf",&bianguige_table[iRow].wide,
					&bianguige_table[iRow].thick,&bianguige_table[iRow].theroy_weight);
			}
		}
		fclose(fp);
	}
	catch(char* sError)
	{
		if(bPrompt)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}

BOOL SlotGuiGeSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt)
{
	FILE *fp;
	char file_path[MAX_PATH],line_txt[500];
	sprintf(file_path,"%s",sFileName);
	if(strlen(file_path)<=0)
		return FALSE;
	int iRow;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw CString("Can't open U-steel's specification database,the database remains unchanged.");
#else 
				throw CString("打不开槽钢规格库文件,槽钢规格库保持不变");
#endif
			fprintf(fp,"%d \n",slotguige_N);
			for(iRow=0;iRow<slotguige_N;iRow++)
			{
				sprintf(line_txt,"%s\n",slotguige_table[iRow].guige);	//槽钢规格单独写一行，方便处理
				fputs(line_txt,fp);
				//高h; 宽b; 厚d; 肢厚t; 内圆半径r; 截面积area; 理论重量; Ix(cm4); Rx(cm); Wx(cm3); Iy(cm4); Ry(cm); Wy(cm3); Z0(cm)
				fprintf(fp,"%8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f",slotguige_table[iRow].h,
					slotguige_table[iRow].b,slotguige_table[iRow].d,slotguige_table[iRow].t,slotguige_table[iRow].r,
					slotguige_table[iRow].area,slotguige_table[iRow].theroy_weight,slotguige_table[iRow].Ix,
					slotguige_table[iRow].Rx,slotguige_table[iRow].Wx,slotguige_table[iRow].Iy,
					slotguige_table[iRow].Ry,slotguige_table[iRow].Wy,slotguige_table[iRow].Z0);
				fprintf(fp,"\n");
			}
		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
				return FALSE;
#ifdef AFX_TARG_ENU_ENGLISH
				//throw CString("Can't open U-steel's specification database,the database will be default.");
#else 
				//throw CString("打不开槽钢规格库文件,槽钢规格将缺省");
#endif
			if(fp==NULL)
				return FALSE;
			fscanf(fp,"%d",&slotguige_N);
			if(slotguige_N>100)
			{
				fclose(fp);
#ifdef AFX_TARG_ENU_ENGLISH
				throw CString("The system can only handle 100 specifications of U-steel at the same time");
#else 
				throw CString("系统只能同时处理100种槽钢规格");
#endif
			}
			fgets(line_txt,100,fp);
			for(iRow=0;iRow<slotguige_N;iRow++)
			{
				fgets(line_txt,16,fp);
				sscanf(line_txt,"%s",slotguige_table[iRow].guige);
				int str_len=strlen(slotguige_table[iRow].guige);
				if(slotguige_table[iRow].guige[str_len-1]=='\n')
					slotguige_table[iRow].guige[str_len-1]='\0';
				fgets(line_txt,500,fp);
				//高h; 宽b; 厚d; 肢厚t; 内圆半径r; 截面积area; 理论重量; Ix(cm4); Rx(cm); Wx(cm3); Iy(cm4); Ry(cm); Wy(cm3); Z0(cm)
				sscanf(line_txt,"%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",&slotguige_table[iRow].h,
					&slotguige_table[iRow].b,&slotguige_table[iRow].d,&slotguige_table[iRow].t,&slotguige_table[iRow].r,
					&slotguige_table[iRow].area,&slotguige_table[iRow].theroy_weight,&slotguige_table[iRow].Ix,
					&slotguige_table[iRow].Rx,&slotguige_table[iRow].Wx,&slotguige_table[iRow].Iy,
					&slotguige_table[iRow].Ry,&slotguige_table[iRow].Wy,&slotguige_table[iRow].Z0);
			}
		}
		fclose(fp);
	}
	catch(char* sError)
	{
		if(bPrompt)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}
#ifdef __H_SHAPE_STEEL_
BOOL HShapeGuiGeSerialize(CString sFileName,int iPartSubType, BOOL bWrite,BOOL bPrompt)
{
	FILE *fp;
	char file_path[MAX_PATH],line_txt[500];
	sprintf(file_path,"%s",sFileName);
	if(strlen(file_path)<=0)
		return FALSE;
	int iRow;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw CString("Can't open U-steel's specification database,the database remains unchanged.");
#else 
				throw CString("打不开H形刚规格库文件,H形钢规格库保持不变");
#endif
			if(iPartSubType==0)
			{
				fprintf(fp,"%d \n",hw_guige_N);
				for(iRow=0;iRow<hw_guige_N;iRow++)
				{
					//高h; 宽b; 腹板厚度t1; 翼缘厚度t2; 内圆半径r; 截面积area; 理论重量; Ix(cm4); Iy(cm4);Rx(cm); Ry(cm); Wx(cm3);  Wy(cm3);
					if(iPartSubType==0)
						fprintf(fp,"%8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f",hw_guige_table[iRow].h,
						hw_guige_table[iRow].b,hw_guige_table[iRow].t1,hw_guige_table[iRow].t2,hw_guige_table[iRow].r,
						hw_guige_table[iRow].area,hw_guige_table[iRow].theroy_weight,hw_guige_table[iRow].Ix,
						hw_guige_table[iRow].Iy,hw_guige_table[iRow].Rx,hw_guige_table[iRow].Ry,
						hw_guige_table[iRow].Wx,hw_guige_table[iRow].Wy);
					fprintf(fp,"\n");
				}
			}
			else if(iPartSubType==1)
			{
				fprintf(fp,"%d \n",hm_guige_N);
				for(iRow=0;iRow<hm_guige_N;iRow++)
				{
					//高h; 宽b; 腹板厚度t1; 翼缘厚度t2; 内圆半径r; 截面积area; 理论重量; Ix(cm4); Iy(cm4);Rx(cm); Ry(cm); Wx(cm3);  Wy(cm3);
						fprintf(fp,"%8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f",hm_guige_table[iRow].h,
						hm_guige_table[iRow].b,hm_guige_table[iRow].t1,hm_guige_table[iRow].t2,hm_guige_table[iRow].r,
						hm_guige_table[iRow].area,hm_guige_table[iRow].theroy_weight,hm_guige_table[iRow].Ix,
						hm_guige_table[iRow].Iy,hm_guige_table[iRow].Rx,hm_guige_table[iRow].Ry,
						hm_guige_table[iRow].Wx,hm_guige_table[iRow].Wy);
					fprintf(fp,"\n");
				}
			}
			else if(iPartSubType==2)
			{
				fprintf(fp,"%d \n",hn_guige_N);
				for(iRow=0;iRow<hn_guige_N;iRow++)
				{
					//高h; 宽b; 腹板厚度t1; 翼缘厚度t2; 内圆半径r; 截面积area; 理论重量; Ix(cm4); Iy(cm4);Rx(cm); Ry(cm); Wx(cm3);  Wy(cm3);
				fprintf(fp,"%8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f",hn_guige_table[iRow].h,
					hn_guige_table[iRow].b,hn_guige_table[iRow].t1,hn_guige_table[iRow].t2,hn_guige_table[iRow].r,
					hn_guige_table[iRow].area,hn_guige_table[iRow].theroy_weight,hn_guige_table[iRow].Ix,
					hn_guige_table[iRow].Iy,hn_guige_table[iRow].Rx,hn_guige_table[iRow].Ry,
					hn_guige_table[iRow].Wx,hn_guige_table[iRow].Wy);
					fprintf(fp,"\n");
				}
			}
			else if(iPartSubType==3)
			{
				fprintf(fp,"%d \n",hp_guige_N);
				for(iRow=0;iRow<hp_guige_N;iRow++)
				{
					//高h; 宽b; 腹板厚度t1; 翼缘厚度t2; 内圆半径r; 截面积area; 理论重量; Ix(cm4); Iy(cm4);Rx(cm); Ry(cm); Wx(cm3);  Wy(cm3);
					fprintf(fp,"%8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f",hp_guige_table[iRow].h,
						hp_guige_table[iRow].b,hp_guige_table[iRow].t1,hp_guige_table[iRow].t2,hp_guige_table[iRow].r,
						hp_guige_table[iRow].area,hp_guige_table[iRow].theroy_weight,hp_guige_table[iRow].Ix,
						hp_guige_table[iRow].Iy,hp_guige_table[iRow].Rx,hp_guige_table[iRow].Ry,
						hp_guige_table[iRow].Wx,hp_guige_table[iRow].Wy);
					fprintf(fp,"\n");
				}
			}
		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw CString("Can't open U-steel's specification database,the database will be default.");
#else 
				throw CString("打不开H形钢规格库文件,H形钢规格将缺省");
#endif
			if(iPartSubType==0)
			{
				fscanf(fp,"%d",&hw_guige_N);
				if(hw_guige_N>100)
				{
					fclose(fp);
#ifdef AFX_TARG_ENU_ENGLISH
					throw CString("The system can only handle 100 specifications of U-steel at the same time");
#else 
					throw CString("系统只能同时处理100种H形钢规格");
#endif
				}
				fgets(line_txt,100,fp);
				for(iRow=0;iRow<hw_guige_N;iRow++)
				{
					fgets(line_txt,500,fp);
					//高h; 宽b; 腹板厚度t1; 翼缘厚度t2; 内圆半径r; 截面积area; 理论重量; Ix(cm4); Iy(cm4);Rx(cm); Ry(cm); Wx(cm3);  Wy(cm3);
					sscanf(line_txt,"%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",&hw_guige_table[iRow].h,
						&hw_guige_table[iRow].b,&hw_guige_table[iRow].t1,&hw_guige_table[iRow].t2,&hw_guige_table[iRow].r,
						&hw_guige_table[iRow].area,&hw_guige_table[iRow].theroy_weight,&hw_guige_table[iRow].Ix,
						&hw_guige_table[iRow].Iy,&hw_guige_table[iRow].Rx,&hw_guige_table[iRow].Ry,
						&hw_guige_table[iRow].Wx,&hw_guige_table[iRow].Wy);
				}
			}
			else if(iPartSubType==1)
			{
				fscanf(fp,"%d",&hm_guige_N);
				if(hm_guige_N>100)
				{
					fclose(fp);
#ifdef AFX_TARG_ENU_ENGLISH
					throw CString("The system can only handle 100 specifications of U-steel at the same time");
#else 
					throw CString("系统只能同时处理100种H形钢规格");
#endif
				}
				fgets(line_txt,100,fp);
				for(iRow=0;iRow<hm_guige_N;iRow++)
				{
					fgets(line_txt,500,fp);
					//高h; 宽b; 腹板厚度t1; 翼缘厚度t2; 内圆半径r; 截面积area; 理论重量; Ix(cm4); Iy(cm4);Rx(cm); Ry(cm); Wx(cm3);  Wy(cm3);
					sscanf(line_txt,"%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",&hm_guige_table[iRow].h,
						&hm_guige_table[iRow].b,&hm_guige_table[iRow].t1,&hm_guige_table[iRow].t2,&hm_guige_table[iRow].r,
						&hm_guige_table[iRow].area,&hm_guige_table[iRow].theroy_weight,&hm_guige_table[iRow].Ix,
						&hm_guige_table[iRow].Iy,&hm_guige_table[iRow].Rx,&hm_guige_table[iRow].Ry,
						&hm_guige_table[iRow].Wx,&hm_guige_table[iRow].Wy);
				}
			}
			else if(iPartSubType==2)
			{
				fscanf(fp,"%d",&hn_guige_N);
				if(hn_guige_N>100)
				{
					fclose(fp);
#ifdef AFX_TARG_ENU_ENGLISH
					throw CString("The system can only handle 100 specifications of U-steel at the same time");
#else 
					throw CString("系统只能同时处理100种H形钢规格");
#endif
				}
				fgets(line_txt,100,fp);
				for(iRow=0;iRow<hn_guige_N;iRow++)
				{
					fgets(line_txt,500,fp);
					//高h; 宽b; 腹板厚度t1; 翼缘厚度t2; 内圆半径r; 截面积area; 理论重量; Ix(cm4); Iy(cm4);Rx(cm); Ry(cm); Wx(cm3);  Wy(cm3);
					sscanf(line_txt,"%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",&hn_guige_table[iRow].h,
						&hn_guige_table[iRow].b,&hn_guige_table[iRow].t1,&hn_guige_table[iRow].t2,&hn_guige_table[iRow].r,
						&hn_guige_table[iRow].area,&hn_guige_table[iRow].theroy_weight,&hn_guige_table[iRow].Ix,
						&hn_guige_table[iRow].Iy,&hn_guige_table[iRow].Rx,&hn_guige_table[iRow].Ry,
						&hn_guige_table[iRow].Wx,&hn_guige_table[iRow].Wy);
				}
			}
			else if(iPartSubType==3)
			{
				fscanf(fp,"%d",&hp_guige_N);
				if(hp_guige_N>100)
				{
					fclose(fp);
#ifdef AFX_TARG_ENU_ENGLISH
					throw CString("The system can only handle 100 specifications of U-steel at the same time");
#else 
					throw CString("系统只能同时处理100种H形钢规格");
#endif
				}
				fgets(line_txt,100,fp);
				for(iRow=0;iRow<hp_guige_N;iRow++)
				{
					fgets(line_txt,500,fp);
					//高h; 宽b; 腹板厚度t1; 翼缘厚度t2; 内圆半径r; 截面积area; 理论重量; Ix(cm4); Iy(cm4);Rx(cm); Ry(cm); Wx(cm3);  Wy(cm3);
					sscanf(line_txt,"%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",&hp_guige_table[iRow].h,
						&hp_guige_table[iRow].b,&hp_guige_table[iRow].t1,&hp_guige_table[iRow].t2,&hp_guige_table[iRow].r,
						&hp_guige_table[iRow].area,&hp_guige_table[iRow].theroy_weight,&hp_guige_table[iRow].Ix,
						&hp_guige_table[iRow].Iy,&hp_guige_table[iRow].Rx,&hp_guige_table[iRow].Ry,
						&hp_guige_table[iRow].Wx,&hp_guige_table[iRow].Wy);
				}
			}
		}
		fclose(fp);
	}
	catch(char* sError)
	{
		if(bPrompt)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}
#endif
BOOL JgJointParaSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt)
{
	FILE *fp;
	char file_path[MAX_PATH];
	sprintf(file_path,"%s",sFileName);
	if(strlen(file_path)<=0)
		return FALSE;
	int iRow;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw CString("Can't open angle joint's database,the database remains unchanged");
#else 
				throw CString("打不开角钢接头库文件,角钢接头库保持不变");
#endif
			fprintf(fp,"%d \n",g_nJgJointRecordNum);
			for(iRow=0;iRow<g_nJgJointRecordNum;iRow++)
			{	//基准角钢宽度 fBaseJgWidth;基准角钢厚度fBaseJgThick;内包角钢宽度fInnerJgWidth;内包角钢厚度fInnerJgThick;
				//外包钢板宽度fOuterPlateWidth;外包钢板厚度fOuterPlateThick;外包角钢宽度fOuterJgWidth;外包角钢厚度fOuterJgThick
				fprintf(fp,"%8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f",
					jg_joint_table[iRow].fBaseJgWidth,jg_joint_table[iRow].fBaseJgThick,
					jg_joint_table[iRow].fInnerJgWidth,jg_joint_table[iRow].fInnerJgThick,
					jg_joint_table[iRow].fOuterPlateWidth,jg_joint_table[iRow].fOuterPlateThick,
					jg_joint_table[iRow].fOuterJgWidth,jg_joint_table[iRow].fOuterJgThick);
				fprintf(fp,"\n");
			}
		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw CString("Can't open angle joint's database,the database will be default.");
#else 
				throw CString("打不开角钢接头库文件,角钢接头将缺省");
#endif
			fscanf(fp,"%d",&g_nJgJointRecordNum);
			if(g_nJgJointRecordNum>200)
			{
				fclose(fp);
#ifdef AFX_TARG_ENU_ENGLISH
				throw CString("The system can only handle 200 parameters of angle joint at the same time");
#else 
				throw CString("系统只能同时处理200种角钢接头参数");
#endif
			}
			for(iRow=0;iRow<g_nJgJointRecordNum;iRow++)
			{	//基准角钢宽度 fBaseJgWidth;基准角钢厚度fBaseJgThick;内包角钢宽度fInnerJgWidth;内包角钢厚度fInnerJgThick;
				//外包钢板宽度fOuterPlateWidth;外包钢板厚度fOuterPlateThick;外包角钢宽度fOuterJgWidth;外包角钢厚度fOuterJgThick
				fscanf(fp,"%lf %lf %lf %lf %lf %lf %lf %lf",
					&jg_joint_table[iRow].fBaseJgWidth,&jg_joint_table[iRow].fBaseJgThick,
					&jg_joint_table[iRow].fInnerJgWidth,&jg_joint_table[iRow].fInnerJgThick,
					&jg_joint_table[iRow].fOuterPlateWidth,&jg_joint_table[iRow].fOuterPlateThick,
					&jg_joint_table[iRow].fOuterJgWidth,&jg_joint_table[iRow].fOuterJgThick);
			}
		}
		fclose(fp);
	}
	catch(char* sError)
	{
		if(bPrompt)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}

BOOL LsGuiGeSerialize(CLsFamily* pLsFamily,CString sFileName, BOOL bWrite,BOOL bPrompt)
{
	FILE *fp;
	char file_path[MAX_PATH],line_txt[100];
	sprintf(file_path,"%s",sFileName);
	if(strlen(file_path)<=0)
		return FALSE;
	int iRow;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open bolt's specification database,the database remains unchanged.";
#else 
				throw "打不开螺栓规格库文件,螺栓规格库保持不变";
#endif
			if(pLsFamily)
			{
				fprintf(fp,"%d \n",pLsFamily->GetCount());
				for(iRow=0;iRow<pLsFamily->GetCount();iRow++)
				{
					LS_XING_HAO *pXingHao=pLsFamily->RecordAt(iRow);
					sprintf(line_txt,"%.1f %4d %4d %4d %4d %4d %4d %lf\n",pXingHao->grade,
						pXingHao->d,pXingHao->valid_len,
						pXingHao->no_thread_len,pXingHao->down_lim,
						pXingHao->up_lim,pXingHao->washer_thick,pXingHao->weight);
					fputs(line_txt,fp);
				}
			}
		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open bolt's specification database,the database will be default";
#else 
				throw "打不开螺栓规格库文件,螺栓规格将缺省";
#endif
			if(pLsFamily)
			{
				int record_n;
				fscanf(fp,"%d",&record_n);
				fgets(line_txt,100,fp);
				pLsFamily->CreateFamily(record_n);
				for(iRow=0;iRow<record_n;iRow++)
				{
					LS_XING_HAO *pXingHao=pLsFamily->RecordAt(iRow);
					fgets(line_txt,100,fp);
					sscanf(line_txt,"%lf%d%d%d%d%d%d%lf",&pXingHao->grade,
						&pXingHao->d,&pXingHao->valid_len,
						&pXingHao->no_thread_len,&pXingHao->down_lim,
						&pXingHao->up_lim,&pXingHao->washer_thick,&pXingHao->weight);
					sprintf(pXingHao->guige,"%.1fM%dX%d",pXingHao->grade,pXingHao->d,pXingHao->valid_len);
				}
			}
		}
		fclose(fp);
	}
	catch(char* sError)
	{
		if(bPrompt)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}
BOOL BoltFamilySerialize(CLsFamily* pLsFamily,CString sFileName, BOOL bWrite,BOOL bPrompt)
{
	FILE *fp;
	char file_path[MAX_PATH],line_txt[MAX_PATH];
	sprintf(file_path,"%s",sFileName);
	if(strlen(file_path)<=0||pLsFamily==NULL)
		return FALSE;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open bolt's specification database,the database remains unchanged.";
#else 
				throw "打不开螺栓零件库文件";
#endif
			fprintf(fp,"LsSubPart-Info\n");
			fprintf(fp,"<serial=1  name=普通螺母>\n");
			fprintf(fp,"<serial=2  name=扣紧螺母>\n");
			fprintf(fp,"<serial=3  name=防盗螺母>\n");
			fprintf(fp,"<serial=4  name=薄螺母>\n");
			fprintf(fp,"<serial=5  name=平垫圈>\n");
			fprintf(fp,"<serial=6  name=弹簧垫圈>\n");
			fprintf(fp,"<serial=7  name=放松扣>\n");
			fprintf(fp,"Base-Info\n");
			fprintf(fp,"id=%d  \n",pLsFamily->GetFamilyHandle());
			fprintf(fp,"name=%s  \n",pLsFamily->name);
			fprintf(fp,"notes=%s  \n",pLsFamily->descr);
			fprintf(fp,"bFootNail=%d  \n",pLsFamily->m_bFootNail);
			fprintf(fp,"Component-Info \n");
			for(int i=0;i<8;i++)
			{
				if(pLsFamily->ls_sub_part[i].idSubPart==0)
					continue;
				sprintf(line_txt,"%d %4d\n",pLsFamily->ls_sub_part[i].idSubPart,pLsFamily->ls_sub_part[i].nCount);
				fputs(line_txt,fp);
			}
			fprintf(fp,"XingHao-Info \n");
			fprintf(fp,"%d \n",pLsFamily->GetCount());
			for(int iRow=0;iRow<pLsFamily->GetCount();iRow++)
			{
				LS_XING_HAO *pXingHao=pLsFamily->RecordAt(iRow);
				sprintf(line_txt,"%.1f %4d %4d %4d %4d %4d %4d %lf\n",pXingHao->grade,
					pXingHao->d,pXingHao->valid_len,
					pXingHao->no_thread_len,pXingHao->down_lim,
					pXingHao->up_lim,pXingHao->washer_thick,pXingHao->weight);
				fputs(line_txt,fp);
			}
		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open bolt's specification database,the database will be default";
#else 
				throw "打不开螺栓零件库文件";
#endif
			IBoltLibrary* pBoltLib=IBoltLibrary::CreateBoltLibrary();
			CLsFamily* pFamily=NULL;
			char key_word[100];
			int hFamily=0;
			int index=0;
			while(!feof(fp))
			{
				if(fgets(line_txt,MAX_PATH,fp)==NULL)
					break;
				if(strstr(line_txt,"XingHao"))
					break;
				if(strstr(line_txt,"Component")||strstr(line_txt,"Base")||strstr(line_txt,"<"))
					continue;
				char sText[MAX_PATH];
				strcpy(sText,line_txt);
				CString sLine=sText;
				sLine.Replace('=',' ');
				sprintf(line_txt,"%s",sLine);
				char *skey=strtok((char*)sText,"=,;");
				strncpy(key_word,skey,100);
				if(stricmp(key_word,"id")==0)
				{
					sscanf(line_txt,"%s%d",key_word,&hFamily);
					pFamily=pBoltLib->AppendFamily(hFamily);
				}
				else if(stricmp(key_word,"name")==0)
					sscanf(line_txt,"%s%s",key_word,pFamily->name);
				else if(stricmp(key_word,"notes")==0)
					sscanf(line_txt,"%s%s",key_word,pFamily->descr);
				else if(stricmp(key_word,"bFootNail")==0)
					sscanf(line_txt,"%s%d",key_word,&pFamily->m_bFootNail);
				else
				{
					sscanf(line_txt,"%d%d",&pFamily->ls_sub_part[index].idSubPart,&pFamily->ls_sub_part[index].nCount);
					index++;
				}
			}
			//螺栓型号
			int record_n=0;
			fgets(line_txt,MAX_PATH,fp);
			sscanf(line_txt,"%d",&record_n);
			pFamily->CreateFamily(record_n);
			for(int iRow=0;iRow<record_n;iRow++)
			{
				LS_XING_HAO *pXingHao=pFamily->RecordAt(iRow);
				fgets(line_txt,100,fp);
				sscanf(line_txt,"%lf%d%d%d%d%d%d%lf",&pXingHao->grade,
					&pXingHao->d,&pXingHao->valid_len,
					&pXingHao->no_thread_len,&pXingHao->down_lim,
					&pXingHao->up_lim,&pXingHao->washer_thick,&pXingHao->weight);
				sprintf(pXingHao->guige,"%.1fM%dX%d",pXingHao->grade,pXingHao->d,pXingHao->valid_len);
			}
			//
			pBoltLib->CopyLsFamily(pLsFamily,pFamily);
			IBoltLibrary::DestroyBoltLibrary(pBoltLib->GetSerialId());
		}
		fclose(fp);
	}
	catch(char* sError)
	{
		if(bPrompt)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}
BOOL LsSubPartSerialize(CLsSubPartFamily* pFamily,CString sFileName, BOOL bWrite,BOOL bPrompt)
{
	FILE *fp;
	char file_path[MAX_PATH],line_txt[100];
	sprintf(file_path,"%s",sFileName);
	if(strlen(file_path)<=0||pFamily==NULL)
		return FALSE;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open bolt's specification database,the database remains unchanged.";
#else 
				throw "打不开螺栓零件库文件";
#endif
			
			fprintf(fp,"%d \n",pFamily->GetCount());
			for(int iRow=0;iRow<pFamily->GetCount();iRow++)
			{
				LS_SUB_PART *pLsSubPart=pFamily->RecordAt(iRow);
				sprintf(line_txt,"%4d %lf %lf\n",pLsSubPart->wiLsD,pLsSubPart->thick,pLsSubPart->weight);
				fputs(line_txt,fp);
			}
		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open bolt's specification database,the database will be default";
#else 
				throw "打不开螺栓零件库文件";
#endif
			int record_n;
			fscanf(fp,"%d",&record_n);
			fgets(line_txt,100,fp);
			pFamily->CreateLsSubPartFamily(record_n);
			for(int iRow=0;iRow<record_n;iRow++)
			{
				LS_SUB_PART *pLsSubPart=pFamily->RecordAt(iRow);
				fgets(line_txt,100,fp);
				sscanf(line_txt,"%d%lf%lf",&pLsSubPart->wiLsD,&pLsSubPart->thick,&pLsSubPart->weight);
			}
		}
		fclose(fp);
	}
	catch(char* sError)
	{
		if(bPrompt)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}
BOOL LsSpaceSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt)
{
	FILE *fp;
	char file_path[MAX_PATH],line_txt[100];
	sprintf(file_path,"%s",sFileName);
	if(strlen(file_path)<=0)
		return FALSE;
	int iRow;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open bolt's spacing margin database,the database remains unchanged";
#else 
				throw "打不开螺栓间距边距库文件,螺栓间距边距库保持不变";
#endif
			fprintf(fp,"%d \n",lsspace_N);
			for(iRow=0;iRow<lsspace_N;iRow++)
			{
				LSSPACEEX boltSpaceEx;
				IBoltLibrary::GetLsSpaceEx(LSSPACE_TABLE[iRow].M_Diameter,&boltSpaceEx);
				//螺栓直径，单排间距，双排间距，端距，连板边距，轧制边距，切角边距
				sprintf(line_txt,"%4d %4d %4d %4d %4d %4d %4d %4d\n",LSSPACE_TABLE[iRow].M_Diameter,
					boltSpaceEx.SingleRowSpace,boltSpaceEx.doubleRowSpace,boltSpaceEx.EndSpace,
					boltSpaceEx.PlateEdgeSpace,boltSpaceEx.RollingEdgeSpace,boltSpaceEx.CutAngleSpace,
					boltSpaceEx.WasherEdgeSpace);
				fputs(line_txt,fp);
			}
		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open bolt's spacing margin database,the database will be default!";
#else 
				throw "打不开螺栓间距边距库文件,螺栓间距边距库将缺省！";
#endif
			fscanf(fp,"%d",&lsspace_N);
			if(lsspace_N>15)
			{
				fclose(fp);
#ifdef AFX_TARG_ENU_ENGLISH
				throw CString("System can only handle 15 spacing margin of bolt at the same time.");
#else 
				throw CString("系统只能同时处理15种螺栓间距！");
#endif
			}
			fgets(line_txt,100,fp);
			for(iRow=0;iRow<lsspace_N;iRow++)
			{
				fgets(line_txt,100,fp);
				int washerSpace=0;
				sscanf(line_txt,"%d%d%d%d%d%d%d%d",&LSSPACE_TABLE[iRow].M_Diameter,
					&LSSPACE_TABLE[iRow].SingleRowSpace,&LSSPACE_TABLE[iRow].doubleRowSpace,&LSSPACE_TABLE[iRow].EndSpace,
					&LSSPACE_TABLE[iRow].PlateEdgeSpace,&LSSPACE_TABLE[iRow].RollingEdgeSpace,&LSSPACE_TABLE[iRow].CutAngleSpace,
					&washerSpace);
				LSSPACEEX boltSpaceEx(LSSPACE_TABLE[iRow]);
				if(washerSpace>0)	//旧版导出的螺栓边距文件没有垫板边距项, washerSpace有可能为零,此时就默认当前连板边距值 wjh-2017.3.16
					boltSpaceEx.WasherEdgeSpace=boltSpaceEx.PlateEdgeSpace;
				IBoltLibrary::SetLsSpaceEx(boltSpaceEx.M_Diameter,boltSpaceEx);
			}
		}
		fclose(fp);
	}
	catch(char* sError)
	{
		if(bPrompt)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}

BOOL CInsertPlateSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt)
{
	FILE *fp=NULL;
	char file_path[MAX_PATH+1],line_txt[500],bolt_txt[200];
	_snprintf(file_path,MAX_PATH,"%s",sFileName);
	if(strlen(file_path)<=0)
		return FALSE;
	int iRow=0;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open trough board's database,the database remain unchanged";
#else 
				throw "打不开槽型插板库文件,槽型插板库保持不变";
#endif
			//1002 增加插板级别 钢管应力系数
			//1003 增加主管材质 用来确定插板类型 wht 10-01-21
			bool bCodeMode=false;
			if(bCodeMode)
			{	//仅用于更析代码中数据段用
				fprintf(fp,"%d \n",rollend_param_N);
				CXhChar500 lineText;
				for(iRow=0;iRow<rollend_param_N;iRow++)
				{	
					lineText.Printf("{%s,%s",(char*)CXhChar16(rollend_param_table[iRow].d),(char*)CXhChar16(rollend_param_table[iRow].t));
					lineText.Append(CXhChar16(",'%C',%d",rollend_param_table[iRow].cMaterial,rollend_param_table[iRow].iLevelMark));
					lineText.Append(CXhChar50(",\"%s\"",rollend_param_table[iRow].codeName));
					lineText.Append(",\"\",\"\",\"\"");
					lineText.Append(CXhChar16(",'%C'",rollend_param_table[iRow].cTubeMat));
					lineText.Append(",");lineText.Append(CXhChar16(rollend_param_table[iRow].L));
					lineText.Append(",");lineText.Append(CXhChar16(rollend_param_table[iRow].M));
					lineText.Append(",");lineText.Append(CXhChar16(rollend_param_table[iRow].H));
					lineText.Append(",");lineText.Append(CXhChar16(rollend_param_table[iRow].W));
					lineText.Append(",");lineText.Append(CXhChar16(rollend_param_table[iRow].N));
					lineText.Append(",");lineText.Append(CXhChar16(rollend_param_table[iRow].S));
					lineText.Append(CXhChar16(",%d",rollend_param_table[iRow].Thick));
					lineText.Append(",");lineText.Append(CXhChar16(rollend_param_table[iRow].theroy_weight));
					lineText.Append(CXhChar16(",0,0,%d",rollend_param_table[iRow].iBoltRect));
					//布置螺栓相关参数
					for(int i=0;i<rollend_param_table[iRow].iBoltRect;i++)
					{
						lineText.Append(CXhChar50(",%d,%d,%d,%d",rollend_param_table[iRow].ls_param[i].iBoltNum,rollend_param_table[iRow].ls_param[i].iRow,
							rollend_param_table[iRow].ls_param[i].iColumn,rollend_param_table[iRow].ls_param[i].iBoltD));
						lineText.Append(",");lineText.Append(CXhChar16(rollend_param_table[iRow].ls_param[i].fBoltPosX));
						lineText.Append(",");lineText.Append(CXhChar16(rollend_param_table[iRow].ls_param[i].fBoltPosY));
						lineText.Append(",");lineText.Append(CXhChar16(rollend_param_table[iRow].ls_param[i].fHorzSpace));
						lineText.Append(",");lineText.Append(CXhChar16(rollend_param_table[iRow].ls_param[i].fVertSpace));
					}
					lineText.Append("},\n");
					fputs(lineText,fp);
				}
				fclose(fp);
				return TRUE;
			}
			fprintf(fp,"%d \n",1003);	//写入版本号 兼容以前定义的插板库  wht 09-09-14
			fprintf(fp,"%d \n",rollend_param_N);
			for(iRow=0;iRow<rollend_param_N;iRow++)
			{	
				sprintf(line_txt,"%s\n",rollend_param_table[iRow].codeName);	//插板代号单独写一行，方便处理
				fputs(line_txt,fp);
				//1002版本增加插板级别	wht 10-01-19
				//直径 管厚 主管材质 板材质 插板级别 L M H W N S Thick 重量 螺栓矩阵个数 
				_snprintf(line_txt,499,"%lf %lf %c %c %4d %lf %lf %lf %lf %lf %lf %4d %lf %4d\n",
					rollend_param_table[iRow].d,rollend_param_table[iRow].t,rollend_param_table[iRow].cTubeMat,
					rollend_param_table[iRow].cMaterial,rollend_param_table[iRow].iLevelMark,
					rollend_param_table[iRow].L,rollend_param_table[iRow].M,rollend_param_table[iRow].H,rollend_param_table[iRow].W,
					rollend_param_table[iRow].N,rollend_param_table[iRow].S,rollend_param_table[iRow].Thick,rollend_param_table[iRow].theroy_weight,rollend_param_table[iRow].iBoltRect);
				fputs(line_txt,fp);
				//布置螺栓相关参数
				for(int i=0;i<rollend_param_table[iRow].iBoltRect;i++)
				{
					_snprintf(bolt_txt,199,"%4d %4d %4d %4d %lf %lf %lf %lf\n",
						rollend_param_table[iRow].ls_param[i].iBoltNum,rollend_param_table[iRow].ls_param[i].iRow,
						rollend_param_table[iRow].ls_param[i].iColumn,rollend_param_table[iRow].ls_param[i].iBoltD,
						rollend_param_table[iRow].ls_param[i].fHorzSpace,rollend_param_table[iRow].ls_param[i].fVertSpace,
						rollend_param_table[iRow].ls_param[i].fBoltPosX,rollend_param_table[iRow].ls_param[i].fBoltPosY);
					fputs(bolt_txt,fp);
				}
			}
			//录入库文件时使用
			/*for(iRow=0;iRow<rollend_param_N;iRow++)
			{	//直径 管厚 板材质 规格 插板级别 L M H W N S Thick 重量 螺栓矩阵个数
				int iLevel=1;
				char cMat='S';
				int str_len=strlen(rollend_param_table[iRow].codeName);
				cMat=rollend_param_table[iRow].codeName[str_len-2];
				iLevel=atoi(&rollend_param_table[iRow].codeName[str_len-1]);
				sprintf(line_txt,"{%.f,%.f,'%c',%d,\"%s\",%.f,%.f,%.f,%.f,%.f,%.f,%d,%.2f,%.f,%.f,%d,",
					rollend_param_table[iRow].d,rollend_param_table[iRow].t,cMat,iLevel,
					rollend_param_table[iRow].codeName,
					rollend_param_table[iRow].L,rollend_param_table[iRow].M,rollend_param_table[iRow].H,
					rollend_param_table[iRow].W,rollend_param_table[iRow].N,rollend_param_table[iRow].S,
					rollend_param_table[iRow].Thick,rollend_param_table[iRow].theroy_weight,0.0,
					0.0,rollend_param_table[iRow].iBoltRect);
				fputs(line_txt,fp);
				//布置螺栓相关参数
				for(int i=0;i<rollend_param_table[iRow].iBoltRect;i++)
				{
					if(i<rollend_param_table[iRow].iBoltRect-1)
					{
						sprintf(line_txt,"%d,%d,%d,%d,%.f,%.f,%.f,%.f,",
							rollend_param_table[iRow].ls_param[i].iBoltNum,rollend_param_table[iRow].ls_param[i].iRow,
							rollend_param_table[iRow].ls_param[i].iColumn,rollend_param_table[iRow].ls_param[i].iBoltD,
							rollend_param_table[iRow].ls_param[i].fBoltPosX,rollend_param_table[iRow].ls_param[i].fBoltPosY,
							rollend_param_table[iRow].ls_param[i].fHorzSpace,rollend_param_table[iRow].ls_param[i].fVertSpace);
					}
					else
					{
						sprintf(line_txt,"%d,%d,%d,%d,%.f,%.f,%.f,%.f",
							rollend_param_table[iRow].ls_param[i].iBoltNum,rollend_param_table[iRow].ls_param[i].iRow,
							rollend_param_table[iRow].ls_param[i].iColumn,rollend_param_table[iRow].ls_param[i].iBoltD,
							rollend_param_table[iRow].ls_param[i].fBoltPosX,rollend_param_table[iRow].ls_param[i].fBoltPosY,
							rollend_param_table[iRow].ls_param[i].fHorzSpace,rollend_param_table[iRow].ls_param[i].fVertSpace);
					}
					fputs(line_txt,fp);
				}
				sprintf(line_txt,"},\n");
				fputs(line_txt,fp);
			}*/
		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open through board's database ,the database will be default.";
#else 
				throw "打不开槽型插板库文件,槽型插板库将缺省";
#endif
			int str_len=0,nVersion=0;
			fscanf(fp,"%d",&nVersion);	//尝试读取版本号，新版本第一行存储版本号，旧版本第一行即为插板库记录个数
			fgets(line_txt,500,fp);
			if(nVersion==1001||nVersion==1002||nVersion==1003)
			{	//版本号大于等于1.0.0.1时下一行存取插板库记录个数，否则第一行即为插板库记录个数
				fscanf(fp,"%d",&rollend_param_N);
				fgets(line_txt,500,fp);
			}
			else 
				rollend_param_N=nVersion;
			if(rollend_param_N>200)
			{
				fclose(fp);
#ifdef AFX_TARG_ENU_ENGLISH
				throw "System can only handle 200 through board at the same time";
#else 
				throw "系统只能同时处理200种槽型插板";
#endif
			}
			int iMaxRowNum=rollend_param_N;
			for(iRow=0;iRow<iMaxRowNum;iRow++)
			{
				if(nVersion==1001||nVersion==1002||nVersion==1003)
				{	//大于等于1.0.0.1 
					fgets(line_txt,16,fp);
					sscanf(line_txt,"%s",rollend_param_table[iRow].codeName);
					str_len=strlen(rollend_param_table[iRow].codeName);
					if(rollend_param_table[iRow].codeName[str_len-1]=='\n')
						rollend_param_table[iRow].codeName[str_len-1]='\0';
				}
				fgets(line_txt,500,fp);
				if(nVersion==1003)
				{	//1.0.0.3增加钢管材质	wht 10-01-19
					sscanf(line_txt,"%lf %lf %c %c %4d %lf %lf %lf %lf %lf %lf %4d %lf %4d",
						&rollend_param_table[iRow].d,&rollend_param_table[iRow].t,&rollend_param_table[iRow].cTubeMat,
						&rollend_param_table[iRow].cMaterial,&rollend_param_table[iRow].iLevelMark,
						&rollend_param_table[iRow].L,&rollend_param_table[iRow].M,&rollend_param_table[iRow].H,&rollend_param_table[iRow].W,
						&rollend_param_table[iRow].N,&rollend_param_table[iRow].S,&rollend_param_table[iRow].Thick,&rollend_param_table[iRow].theroy_weight,&rollend_param_table[iRow].iBoltRect);
				}
				else if(nVersion==1002)
				{	//1.0.0.2增加插板级别	wht 10-01-19
					sscanf(line_txt,"%lf %lf %c %4d %lf %lf %lf %lf %lf %lf %4d %lf %4d",
						&rollend_param_table[iRow].d,&rollend_param_table[iRow].t,&rollend_param_table[iRow].cMaterial,&rollend_param_table[iRow].iLevelMark,
						&rollend_param_table[iRow].L,&rollend_param_table[iRow].M,&rollend_param_table[iRow].H,&rollend_param_table[iRow].W,
						&rollend_param_table[iRow].N,&rollend_param_table[iRow].S,&rollend_param_table[iRow].Thick,&rollend_param_table[iRow].theroy_weight,&rollend_param_table[iRow].iBoltRect);
				}
				else
				{
					sscanf(line_txt,"%lf %lf %c %lf %lf %lf %lf %lf %lf %4d %lf %4d",
						&rollend_param_table[iRow].d,&rollend_param_table[iRow].t,&rollend_param_table[iRow].cMaterial,
						&rollend_param_table[iRow].L,&rollend_param_table[iRow].M,&rollend_param_table[iRow].H,&rollend_param_table[iRow].W,
						&rollend_param_table[iRow].N,&rollend_param_table[iRow].S,&rollend_param_table[iRow].Thick,&rollend_param_table[iRow].theroy_weight,&rollend_param_table[iRow].iBoltRect);
				}
				//布置螺栓相关参数
				for(int i=0;i<rollend_param_table[iRow].iBoltRect;i++)
				{
					fgets(bolt_txt,200,fp);
					sscanf(bolt_txt,"%4d %4d %4d %4d %lf %lf %lf %lf",
						&rollend_param_table[iRow].ls_param[i].iBoltNum,&rollend_param_table[iRow].ls_param[i].iRow,//+1
						&rollend_param_table[iRow].ls_param[i].iColumn,&rollend_param_table[iRow].ls_param[i].iBoltD,
						&rollend_param_table[iRow].ls_param[i].fHorzSpace,&rollend_param_table[iRow].ls_param[i].fVertSpace,
						&rollend_param_table[iRow].ls_param[i].fBoltPosX,&rollend_param_table[iRow].ls_param[i].fBoltPosY);
				}
				if(str_len==0||(nVersion!=1001&&nVersion!=1002&&nVersion!=1003))
				{	//! 1.0.0.1
					int d=(int)rollend_param_table[iRow].d;
					int t=(int)rollend_param_table[iRow].t;
					int d_third=0,d_sec=0;	//钢管直径百位数字以及十位数字
					int t_sec=0,t_first=0;	//钢管厚度十位数字以及个位数字
					d_third=d/100;		//百位数字
					d_sec=(d%100)/10;	//十位数字
					t_sec=t/10;			//十位数字
					t_first=t%10;		//个位数字
					sprintf(rollend_param_table[iRow].codeName,"C%d%d%d%d%c%d",d_third,d_sec,t_sec,t_first,
						rollend_param_table[iRow].cTubeMat,rollend_param_table[iRow].iLevelMark);
				}
			}
		}
		fclose(fp);
	}
	catch(char* sError)
	{
		if(bPrompt)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}

BOOL UInsertPlateSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt)
{
	CXhChar500 lineText;
	FILE *fp=NULL;
	char file_path[MAX_PATH+1],line_txt[500],bolt_txt[200];
	_snprintf(file_path,MAX_PATH,"%s",sFileName);
	if(strlen(file_path)<=0)
		return TRUE;
	int iRow=0;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open U-board's database,the database remain unchanged.";
#else 
				throw "打不开U型插板库文件,U型插板库保持不变";
#endif
			//1002 增加插板级别 钢管应力系数
			//1003 增加主管材质 用来确定插板类型 wht 10-01-21
			bool bCodeMode=false;
			if(bCodeMode)
			{	//仅用于更析代码中数据段用
				fprintf(fp,"%d \n",uend_param_N);
				CXhChar500 lineText;
				for(iRow=0;iRow<uend_param_N;iRow++)
				{	
					lineText.Printf("{%s,%s",(char*)CXhChar16(uend_param_table[iRow].d),(char*)CXhChar16(uend_param_table[iRow].t));
					lineText.Append(CXhChar16(",'%C',%d",uend_param_table[iRow].cMaterial,uend_param_table[iRow].iLevelMark));
					lineText.Append(CXhChar50(",\"%s\"",uend_param_table[iRow].codeName));
					lineText.Append(",\"\",\"\",\"\"");
					lineText.Append(CXhChar16(",'%C'",uend_param_table[iRow].cTubeMat));
					lineText.Append(",");lineText.Append(CXhChar16(uend_param_table[iRow].L));
					lineText.Append(",");lineText.Append(CXhChar16(uend_param_table[iRow].M));
					lineText.Append(",");lineText.Append(CXhChar16(uend_param_table[iRow].H));
					lineText.Append(",");lineText.Append(CXhChar16(uend_param_table[iRow].W));
					lineText.Append(",");lineText.Append(CXhChar16(uend_param_table[iRow].N));
					lineText.Append(",");lineText.Append(CXhChar16(uend_param_table[iRow].S));
					lineText.Append(CXhChar16(",%d",uend_param_table[iRow].Thick));
					lineText.Append(",");lineText.Append(CXhChar16(uend_param_table[iRow].theroy_weight));
					lineText.Append(CXhChar16(",0,0,%d",uend_param_table[iRow].iBoltRect));
					//布置螺栓相关参数
					for(int i=0;i<uend_param_table[iRow].iBoltRect;i++)
					{
						lineText.Append(CXhChar50(",%d,%d,%d,%d",uend_param_table[iRow].ls_param[i].iBoltNum,uend_param_table[iRow].ls_param[i].iRow,
							uend_param_table[iRow].ls_param[i].iColumn,uend_param_table[iRow].ls_param[i].iBoltD));
						lineText.Append(",");lineText.Append(CXhChar16(uend_param_table[iRow].ls_param[i].fBoltPosX));
						lineText.Append(",");lineText.Append(CXhChar16(uend_param_table[iRow].ls_param[i].fBoltPosY));
						lineText.Append(",");lineText.Append(CXhChar16(uend_param_table[iRow].ls_param[i].fHorzSpace));
						lineText.Append(",");lineText.Append(CXhChar16(uend_param_table[iRow].ls_param[i].fVertSpace));
					}
					lineText.Append("},\n");
					fputs(lineText,fp);
				}
				fclose(fp);
				return TRUE;
			}
			fprintf(fp,"%d \n",1003);	//写入版本号 兼容以前定义的插板库  wht 09-09-14
			fprintf(fp,"%d \n",uend_param_N);
			for(iRow=0;iRow<uend_param_N;iRow++)
			{	
				_snprintf(line_txt,499,"%s\n",uend_param_table[iRow].codeName);	
				fputs(line_txt,fp);		//插板代号单独写一行，方便处理
				//1.0.0.2 增加插板级别	wht 10-01-19
				//直径 管厚 钢管材质 板材质 插板级别 L M H W N S Thick 重量 螺栓矩阵个数 
				_snprintf(line_txt,499,"%lf %lf %c %c %4d %lf %lf %lf %lf %lf %lf %4d %lf %4d\n",
					uend_param_table[iRow].d,uend_param_table[iRow].t,uend_param_table[iRow].cTubeMat,
					uend_param_table[iRow].cMaterial,uend_param_table[iRow].iLevelMark,
					uend_param_table[iRow].L,uend_param_table[iRow].M,uend_param_table[iRow].H,uend_param_table[iRow].W,
					uend_param_table[iRow].N,uend_param_table[iRow].S,uend_param_table[iRow].Thick,uend_param_table[iRow].theroy_weight,uend_param_table[iRow].iBoltRect);
				fputs(line_txt,fp);
				//布置螺栓相关参数
				for(int i=0;i<uend_param_table[iRow].iBoltRect;i++)
				{
					_snprintf(bolt_txt,199,"%4d %4d %4d %4d %lf %lf %lf %lf\n",
						uend_param_table[iRow].ls_param[i].iBoltNum,uend_param_table[iRow].ls_param[i].iRow,
						uend_param_table[iRow].ls_param[i].iColumn,uend_param_table[iRow].ls_param[i].iBoltD,
						uend_param_table[iRow].ls_param[i].fHorzSpace,uend_param_table[iRow].ls_param[i].fVertSpace,
						uend_param_table[iRow].ls_param[i].fBoltPosX,uend_param_table[iRow].ls_param[i].fBoltPosY);
					fputs(bolt_txt,fp);
				}
			}
			//录入库文件时使用
			/*for(iRow=0;iRow<uend_param_N;iRow++)
			{	//直径 管厚 板材质 规格 插板级别 L M H W N S Thick 重量 螺栓矩阵个数
				int iLevel=1;
				char cMat='S';
				int str_len=strlen(uend_param_table[iRow].codeName);
				cMat=uend_param_table[iRow].codeName[str_len-2];
				iLevel=atoi(&uend_param_table[iRow].codeName[str_len-1]);
				sprintf(line_txt,"{%.f,%.f,'%c',%d,\"%s\",%.f,%.f,%.f,%.f,%.f,%.f,%d,%.2f,%.f,%.f,%d,",
					uend_param_table[iRow].d,uend_param_table[iRow].t,cMat,iLevel,
					uend_param_table[iRow].codeName,
					uend_param_table[iRow].L,uend_param_table[iRow].M,uend_param_table[iRow].H,
					uend_param_table[iRow].W,uend_param_table[iRow].N,uend_param_table[iRow].S,
					uend_param_table[iRow].Thick,uend_param_table[iRow].theroy_weight,0.0,
					0.0,uend_param_table[iRow].iBoltRect);
				fputs(line_txt,fp);
				//布置螺栓相关参数
				for(int i=0;i<uend_param_table[iRow].iBoltRect;i++)
				{
					if(i<uend_param_table[iRow].iBoltRect-1)
					{
						sprintf(line_txt,"%d,%d,%d,%d,%.f,%.f,%.f,%.f,",
							uend_param_table[iRow].ls_param[i].iBoltNum,uend_param_table[iRow].ls_param[i].iRow,
							uend_param_table[iRow].ls_param[i].iColumn,uend_param_table[iRow].ls_param[i].iBoltD,
							uend_param_table[iRow].ls_param[i].fBoltPosX,uend_param_table[iRow].ls_param[i].fBoltPosY,
							uend_param_table[iRow].ls_param[i].fHorzSpace,uend_param_table[iRow].ls_param[i].fVertSpace);
					}
					else 
					{
						sprintf(line_txt,"%d,%d,%d,%d,%.f,%.f,%.f,%.f",
							uend_param_table[iRow].ls_param[i].iBoltNum,uend_param_table[iRow].ls_param[i].iRow,
							uend_param_table[iRow].ls_param[i].iColumn,uend_param_table[iRow].ls_param[i].iBoltD,
							uend_param_table[iRow].ls_param[i].fBoltPosX,uend_param_table[iRow].ls_param[i].fBoltPosY,
							uend_param_table[iRow].ls_param[i].fHorzSpace,uend_param_table[iRow].ls_param[i].fVertSpace);
					}
					fputs(line_txt,fp);
				}
				sprintf(line_txt,"},\n");
				fputs(line_txt,fp);
			}*/
		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open U-board's database,the database will be default";
#else 
				throw "打不开U型插板库文件,U型插板库将缺省";
#endif
			int str_len=0,nVersion=0;
			fscanf(fp,"%d",&nVersion);	//尝试读取版本号，新版本第一行存储版本号，旧版本第一行即为插板库记录个数
			fgets(line_txt,500,fp);
			if(nVersion==1001||nVersion==1002||nVersion==1003)
			{	//版本号为1.0.0.1时下一行存取插板库记录个数，否则第一行即为插板库记录个数
				fscanf(fp,"%d",&uend_param_N);
				fgets(line_txt,500,fp);
			}
			else 
				uend_param_N=nVersion;
			if(uend_param_N>200)
			{
				fclose(fp);
#ifdef AFX_TARG_ENU_ENGLISH
				throw "System can only handle 200 U-board at the same time.";
#else 
				throw "系统只能同时处理200种U型插板";
#endif
			}
			int iMaxRowNum=uend_param_N;
			for(iRow=0;iRow<iMaxRowNum;iRow++)
			{
				if(nVersion==1001||nVersion==1002||nVersion==1003)
				{	//1.0.0.1
					fgets(line_txt,16,fp);
					sscanf(line_txt,"%s",uend_param_table[iRow].codeName);
					str_len=strlen(uend_param_table[iRow].codeName);
					if(uend_param_table[iRow].codeName[str_len-1]=='\n')
						uend_param_table[iRow].codeName[str_len-1]='\0';
				}
				fgets(line_txt,500,fp);
				if(nVersion==1003)
				{	//1.0.0.3 版本增加钢管材质	wht 10-01-19 
					sscanf(line_txt,"%lf %lf %c %c %4d %lf %lf %lf %lf %lf %lf %4d %lf %4d",
						&uend_param_table[iRow].d,&uend_param_table[iRow].t,&uend_param_table[iRow].cTubeMat,
						&uend_param_table[iRow].cMaterial,&uend_param_table[iRow].iLevelMark,
						&uend_param_table[iRow].L,&uend_param_table[iRow].M,&uend_param_table[iRow].H,&uend_param_table[iRow].W,
						&uend_param_table[iRow].N,&uend_param_table[iRow].S,&uend_param_table[iRow].Thick,&uend_param_table[iRow].theroy_weight,&uend_param_table[iRow].iBoltRect);
				}
				else if(nVersion==1002)
				{	//1.0.0.2 版本增加插板级别	wht 10-01-19 
					sscanf(line_txt,"%lf %lf %c %4d %lf %lf %lf %lf %lf %lf %4d %lf %4d",
						&uend_param_table[iRow].d,&uend_param_table[iRow].t,&uend_param_table[iRow].cMaterial,&uend_param_table[iRow].iLevelMark,
						&uend_param_table[iRow].L,&uend_param_table[iRow].M,&uend_param_table[iRow].H,&uend_param_table[iRow].W,
						&uend_param_table[iRow].N,&uend_param_table[iRow].S,&uend_param_table[iRow].Thick,&uend_param_table[iRow].theroy_weight,&uend_param_table[iRow].iBoltRect);
				}
				else
				{
					sscanf(line_txt,"%lf %lf %c %lf %lf %lf %lf %lf %lf %4d %lf %4d",
						&uend_param_table[iRow].d,&uend_param_table[iRow].t,&uend_param_table[iRow].cMaterial,
						&uend_param_table[iRow].L,&uend_param_table[iRow].M,&uend_param_table[iRow].H,&uend_param_table[iRow].W,
						&uend_param_table[iRow].N,&uend_param_table[iRow].S,&uend_param_table[iRow].Thick,&uend_param_table[iRow].theroy_weight,&uend_param_table[iRow].iBoltRect);
				}
				//布置螺栓相关参数
				for(int i=0;i<uend_param_table[iRow].iBoltRect;i++)
				{
					fgets(bolt_txt,200,fp);
					sscanf(bolt_txt,"%4d %4d %4d %4d %lf %lf %lf %lf",
						&uend_param_table[iRow].ls_param[i].iBoltNum,&uend_param_table[iRow].ls_param[i].iRow,//+1
						&uend_param_table[iRow].ls_param[i].iColumn,&uend_param_table[iRow].ls_param[i].iBoltD,
						&uend_param_table[iRow].ls_param[i].fHorzSpace,&uend_param_table[iRow].ls_param[i].fVertSpace,
						&uend_param_table[iRow].ls_param[i].fBoltPosX,&uend_param_table[iRow].ls_param[i].fBoltPosY);
				}
				if(str_len==0||(nVersion!=1001&&nVersion!=1002&&nVersion!=1003))	//! 1.0.0.1
				{
					int d=(int)uend_param_table[iRow].d;
					int t=(int)uend_param_table[iRow].t;
					int d_third=0,d_sec=0;	//钢管直径百位数字以及十位数字
					int t_sec=0,t_first=0;	//钢管厚度十位数字以及个位数字
					d_third=d/100;		//百位数字
					d_sec=(d%100)/10;	//十位数字
					t_sec=t/10;			//十位数字
					t_first=t%10;		//个位数字
					sprintf(uend_param_table[iRow].codeName,"U%d%d%d%d%c%d",d_third,d_sec,t_sec,t_first,
						uend_param_table[iRow].cTubeMat,uend_param_table[iRow].iLevelMark);
				}
			}
		}
		fclose(fp);
	}
	catch(char* sError)
	{
		if(bPrompt)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}

BOOL XInsertPlateSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt)
{
	FILE *fp;
	char file_path[MAX_PATH+1],line_txt[500],bolt_txt[200];
	_snprintf(file_path,MAX_PATH,"%s",sFileName);
	if(strlen(file_path)<=0)
		return FALSE;
	int iRow=0;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open cross board's database,the database remain unchanged.";
#else 
				throw "打不开十字型插板库文件,十字型插板库保持不变";
#endif
			//1002 增加插板级别 钢管应力系数
			//1003 增加主管材质 用来确定插板类型 wht 10-01-21
			//1004 增加基板，垂板，包板编号	wht 10-01-23
			bool bCodeMode=false;
			if(bCodeMode)
			{	//仅用于更析代码中数据段用
				fprintf(fp,"%d \n",xend_param_N);
				CXhChar500 lineText;
				for(iRow=0;iRow<xend_param_N;iRow++)
				{	
					lineText.Printf("{%s,%s",(char*)CXhChar16(xend_param_table[iRow].d),(char*)CXhChar16(xend_param_table[iRow].t));
					lineText.Append(CXhChar16(",'%C',%d",xend_param_table[iRow].cMaterial,xend_param_table[iRow].iLevelMark));
					lineText.Append(CXhChar50(",\"%s\"",xend_param_table[iRow].codeName));
					lineText.Append(CXhChar50(",\"%s\"",xend_param_table[iRow].datumPlatePartNo));
					lineText.Append(CXhChar50(",\"%s\"",xend_param_table[iRow].weldPlatePartNo));
					lineText.Append(CXhChar50(",\"%s\"",xend_param_table[iRow].wrapPlatePartNo));
					lineText.Append(CXhChar16(",'%C'",xend_param_table[iRow].cTubeMat));
					lineText.Append(",");lineText.Append(CXhChar16(xend_param_table[iRow].L));
					lineText.Append(",");lineText.Append(CXhChar16(xend_param_table[iRow].M));
					lineText.Append(",");lineText.Append(CXhChar16(xend_param_table[iRow].H));
					lineText.Append(",");lineText.Append(CXhChar16(xend_param_table[iRow].W));
					lineText.Append(",");lineText.Append(CXhChar16(xend_param_table[iRow].N));
					lineText.Append(",");lineText.Append(CXhChar16(xend_param_table[iRow].S));
					lineText.Append(CXhChar16(",%d",xend_param_table[iRow].Thick));
					lineText.Append(",");lineText.Append(CXhChar16(xend_param_table[iRow].theroy_weight));
					lineText.Append(",");lineText.Append(CXhChar16(xend_param_table[iRow].A));
					lineText.Append(",");lineText.Append(CXhChar16(xend_param_table[iRow].R));
					lineText.Append(CXhChar16(",%d",xend_param_table[iRow].iBoltRect));
					//布置螺栓相关参数
					for(int i=0;i<xend_param_table[iRow].iBoltRect;i++)
					{
						lineText.Append(CXhChar50(",%d,%d,%d,%d",xend_param_table[iRow].ls_param[i].iBoltNum,xend_param_table[iRow].ls_param[i].iRow,
							xend_param_table[iRow].ls_param[i].iColumn,xend_param_table[iRow].ls_param[i].iBoltD));
						lineText.Append(",");lineText.Append(CXhChar16(xend_param_table[iRow].ls_param[i].fBoltPosX));
						lineText.Append(",");lineText.Append(CXhChar16(xend_param_table[iRow].ls_param[i].fBoltPosY));
						lineText.Append(",");lineText.Append(CXhChar16(xend_param_table[iRow].ls_param[i].fHorzSpace));
						lineText.Append(",");lineText.Append(CXhChar16(xend_param_table[iRow].ls_param[i].fVertSpace));
					}
					lineText.Append("},\n");
					fputs(lineText,fp);
				}
				fclose(fp);
				return TRUE;
			}
			fprintf(fp,"%d \n",1004);	//写入版本号 兼容以前定义的插板库  wht 09-09-14
			fprintf(fp,"%d \n",xend_param_N);
			for(iRow=0;iRow<xend_param_N;iRow++)
			{	
				_snprintf(line_txt,499,"%s\n",xend_param_table[iRow].codeName);	
				fputs(line_txt,fp);		//插板代号单独写一行，方便处理
				_snprintf(line_txt,499,"%s\n",xend_param_table[iRow].datumPlatePartNo);	
				fputs(line_txt,fp);		//基板编号单独写一行，方便处理
				_snprintf(line_txt,499,"%s\n",xend_param_table[iRow].weldPlatePartNo);	
				fputs(line_txt,fp);		//垂板编号单独写一行，方便处理
				_snprintf(line_txt,499,"%s\n",xend_param_table[iRow].wrapPlatePartNo);	
				fputs(line_txt,fp);		//包板编号单独写一行，方便处理
				//1002版本添加插板级别 wht 10-01-19
				//直径 管厚 钢管材质 板材质 插板级别 L M H W N S Thick 重量 A R 螺栓矩阵个数 
				sprintf(line_txt,"%lf %lf %c %c %4d %lf %lf %lf %lf %lf %lf %4d %lf %lf %lf %4d\n",
					xend_param_table[iRow].d,xend_param_table[iRow].t,xend_param_table[iRow].cTubeMat,
					xend_param_table[iRow].cMaterial,xend_param_table[iRow].iLevelMark,
					xend_param_table[iRow].L,xend_param_table[iRow].M,xend_param_table[iRow].H,xend_param_table[iRow].W,
					xend_param_table[iRow].N,xend_param_table[iRow].S,xend_param_table[iRow].Thick,xend_param_table[iRow].theroy_weight,
					xend_param_table[iRow].A,xend_param_table[iRow].R,xend_param_table[iRow].iBoltRect);
				fputs(line_txt,fp);
				//布置螺栓相关参数
				for(int i=0;i<xend_param_table[iRow].iBoltRect;i++)
				{
					sprintf(bolt_txt,"%4d %4d %4d %4d %lf %lf %lf %lf\n",
						xend_param_table[iRow].ls_param[i].iBoltNum,xend_param_table[iRow].ls_param[i].iRow,
						xend_param_table[iRow].ls_param[i].iColumn,xend_param_table[iRow].ls_param[i].iBoltD,
						xend_param_table[iRow].ls_param[i].fHorzSpace,xend_param_table[iRow].ls_param[i].fVertSpace,
						xend_param_table[iRow].ls_param[i].fBoltPosX,xend_param_table[iRow].ls_param[i].fBoltPosY);
					fputs(bolt_txt,fp);
				}
			}
			//录入库文件时使用
			/*for(iRow=0;iRow<xend_param_N;iRow++)
			{	//直径 管厚 板材质 规格 插板级别 L M H W N S Thick 重量 螺栓矩阵个数
				int iLevel=1;
				char cMat='S';
				int str_len=strlen(xend_param_table[iRow].codeName);
				cMat=xend_param_table[iRow].codeName[str_len-2];
				iLevel=atoi(&xend_param_table[iRow].codeName[str_len-1]);
				sprintf(line_txt,"{%.f,%.f,'%c',%d,\"%s\",%.f,%.f,%.f,%.f,%.f,%.f,%d,%.2f,%.f,%.f,%d,",
					xend_param_table[iRow].d,xend_param_table[iRow].t,cMat,iLevel,
					xend_param_table[iRow].codeName,
					xend_param_table[iRow].L,xend_param_table[iRow].M,xend_param_table[iRow].H,
					xend_param_table[iRow].W,xend_param_table[iRow].N,xend_param_table[iRow].S,
					xend_param_table[iRow].Thick,xend_param_table[iRow].theroy_weight,xend_param_table[iRow].A,
					xend_param_table[iRow].R,xend_param_table[iRow].iBoltRect);
				fputs(line_txt,fp);
				//布置螺栓相关参数
				for(int i=0;i<xend_param_table[iRow].iBoltRect;i++)
				{
					if(i<xend_param_table[iRow].iBoltRect-1)
					{
						sprintf(line_txt,"%d,%d,%d,%d,%.f,%.f,%.f,%.f,",
							xend_param_table[iRow].ls_param[i].iBoltNum,xend_param_table[iRow].ls_param[i].iRow,
							xend_param_table[iRow].ls_param[i].iColumn,xend_param_table[iRow].ls_param[i].iBoltD,
							xend_param_table[iRow].ls_param[i].fBoltPosX,xend_param_table[iRow].ls_param[i].fBoltPosY,
							xend_param_table[iRow].ls_param[i].fHorzSpace,xend_param_table[iRow].ls_param[i].fVertSpace);
					}
					else 
					{
						sprintf(line_txt,"%d,%d,%d,%d,%.f,%.f,%.f,%.f",
							xend_param_table[iRow].ls_param[i].iBoltNum,xend_param_table[iRow].ls_param[i].iRow,
							xend_param_table[iRow].ls_param[i].iColumn,xend_param_table[iRow].ls_param[i].iBoltD,
							xend_param_table[iRow].ls_param[i].fBoltPosX,xend_param_table[iRow].ls_param[i].fBoltPosY,
							xend_param_table[iRow].ls_param[i].fHorzSpace,xend_param_table[iRow].ls_param[i].fVertSpace);
					}
					fputs(line_txt,fp);
				}
				sprintf(line_txt,"},\n");
				fputs(line_txt,fp);
			}*/
		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open cross board's database,the database will be default.";
#else 
				throw "打不开十字板库文件,十字插板库将缺省";
#endif
			int str_len=0,nVersion=0;
			fscanf(fp,"%d",&nVersion);	//尝试读取版本号，新版本第一行存储版本号，旧版本第一行即为插板库记录个数
			fgets(line_txt,500,fp);
			if(nVersion==1001||nVersion==1002||nVersion==1003||nVersion==1004)
			{	//版本号为1.0.0.1时下一行存取插板库记录个数，否则第一行即为插板库记录个数
				fscanf(fp,"%d",&xend_param_N);
				fgets(line_txt,500,fp);
			}
			else 
				xend_param_N=nVersion;
			if(xend_param_N>200)
			{
				fclose(fp);
#ifdef AFX_TARG_ENU_ENGLISH
				throw "System can only handle 200 cross board at the same time.";
#else 
				throw "系统只能同时处理200种十字插板";
#endif
			}
			int iMaxRowNum=xend_param_N;
			for(iRow=0;iRow<iMaxRowNum;iRow++)
			{
				//INSERT_PLATE_PARAM *pTemp=&xend_param_table[iRow];
				if(nVersion==1001||nVersion==1002||nVersion==1003||nVersion==1004)
				{	//1.0.0.1
					fgets(line_txt,16,fp);
					sscanf(line_txt,"%s",xend_param_table[iRow].codeName);
					str_len=strlen(xend_param_table[iRow].codeName);
					if(xend_param_table[iRow].codeName[str_len-1]=='\n')
						xend_param_table[iRow].codeName[str_len-1]='\0';
					if(nVersion==1004)
					{	//基板编号
						fgets(line_txt,16,fp);
						sscanf(line_txt,"%s",xend_param_table[iRow].datumPlatePartNo);
						str_len=strlen(xend_param_table[iRow].datumPlatePartNo);
						if(xend_param_table[iRow].datumPlatePartNo[str_len-1]=='\n')
							xend_param_table[iRow].datumPlatePartNo[str_len-1]='\0';
						//垂板编号
						fgets(line_txt,16,fp);
						sscanf(line_txt,"%s",xend_param_table[iRow].weldPlatePartNo);
						str_len=strlen(xend_param_table[iRow].weldPlatePartNo);
						if(xend_param_table[iRow].weldPlatePartNo[str_len-1]=='\n')
							xend_param_table[iRow].weldPlatePartNo[str_len-1]='\0';
						//包板编号
						fgets(line_txt,16,fp);
						sscanf(line_txt,"%s",xend_param_table[iRow].wrapPlatePartNo);
						str_len=strlen(xend_param_table[iRow].wrapPlatePartNo);
						if(xend_param_table[iRow].wrapPlatePartNo[str_len-1]=='\n')
							xend_param_table[iRow].wrapPlatePartNo[str_len-1]='\0';
					}
				}
				fgets(line_txt,500,fp);
				if(nVersion>=1003) //大于1003版本的十字插板库都应执行以下代码 wht 11-06-10
				{	//1003版本增加钢管材质
					sscanf(line_txt,"%lf %lf %c %c %4d %lf %lf %lf %lf %lf %lf %4d %lf %lf %lf %4d",
						&xend_param_table[iRow].d,&xend_param_table[iRow].t,&xend_param_table[iRow].cTubeMat,
						&xend_param_table[iRow].cMaterial,&xend_param_table[iRow].iLevelMark,
						&xend_param_table[iRow].L,&xend_param_table[iRow].M,&xend_param_table[iRow].H,&xend_param_table[iRow].W,
						&xend_param_table[iRow].N,&xend_param_table[iRow].S,&xend_param_table[iRow].Thick,&xend_param_table[iRow].theroy_weight,
						&xend_param_table[iRow].A,&xend_param_table[iRow].R,&xend_param_table[iRow].iBoltRect);
				}
				else if(nVersion==1002)
				{	//1002版本增加插板级别
					sscanf(line_txt,"%lf %lf %c %4d %lf %lf %lf %lf %lf %lf %4d %lf %lf %lf %4d",
						&xend_param_table[iRow].d,&xend_param_table[iRow].t,&xend_param_table[iRow].cMaterial,&xend_param_table[iRow].iLevelMark,
						&xend_param_table[iRow].L,&xend_param_table[iRow].M,&xend_param_table[iRow].H,&xend_param_table[iRow].W,
						&xend_param_table[iRow].N,&xend_param_table[iRow].S,&xend_param_table[iRow].Thick,&xend_param_table[iRow].theroy_weight,
						&xend_param_table[iRow].A,&xend_param_table[iRow].R,&xend_param_table[iRow].iBoltRect);
				}
				else 
				{
					sscanf(line_txt,"%lf %lf %c %lf %lf %lf %lf %lf %lf %4d %lf %lf %lf %4d",
						&xend_param_table[iRow].d,&xend_param_table[iRow].t,&xend_param_table[iRow].cMaterial,
						&xend_param_table[iRow].L,&xend_param_table[iRow].M,&xend_param_table[iRow].H,&xend_param_table[iRow].W,
						&xend_param_table[iRow].N,&xend_param_table[iRow].S,&xend_param_table[iRow].Thick,&xend_param_table[iRow].theroy_weight,
						&xend_param_table[iRow].A,&xend_param_table[iRow].R,&xend_param_table[iRow].iBoltRect);
				}
				//布置螺栓相关参数
				for(int i=0;i<xend_param_table[iRow].iBoltRect;i++)
				{
					fgets(bolt_txt,200,fp);
					sscanf(bolt_txt,"%4d %4d %4d %4d %lf %lf %lf %lf",
						&xend_param_table[iRow].ls_param[i].iBoltNum,&xend_param_table[iRow].ls_param[i].iRow,//+1
						&xend_param_table[iRow].ls_param[i].iColumn,&xend_param_table[iRow].ls_param[i].iBoltD,
						&xend_param_table[iRow].ls_param[i].fHorzSpace,&xend_param_table[iRow].ls_param[i].fVertSpace,
						&xend_param_table[iRow].ls_param[i].fBoltPosX,&xend_param_table[iRow].ls_param[i].fBoltPosY);
				}
				if(str_len==0||(nVersion!=1001&&nVersion!=1002&&nVersion!=1003))	//! 1.0.0.1
				{
					int d=(int)xend_param_table[iRow].d;
					int t=(int)xend_param_table[iRow].t;
					int d_third=0,d_sec=0;	//钢管直径百位数字以及十位数字
					int t_sec=0,t_first=0;	//钢管厚度十位数字以及个位数字
					d_third=d/100;		//百位数字
					d_sec=(d%100)/10;	//十位数字
					t_sec=t/10;			//十位数字
					t_first=t%10;		//个位数字
					if(strlen(xend_param_table[iRow].codeName)==0)
					{
						sprintf(xend_param_table[iRow].codeName,"X%d%d%d%d%c%d",d_third,d_sec,t_sec,t_first,
							xend_param_table[iRow].cTubeMat,xend_param_table[iRow].iLevelMark);
					}
				}
			}
		}
		fclose(fp);
	}
	catch(char* sError)
	{
		if(bPrompt)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}

BOOL FlDSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt)
{
	FILE *fp;
	char file_path[MAX_PATH],line_txt[500];
	sprintf(file_path,"%s",sFileName);
	if(strlen(file_path)<=0)
		return FALSE;
	int iRow=0;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open welding flange's specification database,the database remains unchanged.";
#else 
				throw "打不开对焊法兰规格库文件,对焊法兰规格库保持不变";
#endif
			bool bCodeMode=false;
			if(bCodeMode)
			{	//仅用于更析代码中数据段用
				fprintf(fp,"%d \n",fld_param_N);
				CXhChar500 lineText;
				for(iRow=0;iRow<fld_param_N;iRow++)
				{	
					lineText.Printf("{%s,%s",(char*)CXhChar16(fld_param_table[iRow].d),(char*)CXhChar16(fld_param_table[iRow].t));
					lineText.Append(CXhChar16(",'%C','%C'",fld_param_table[iRow].cLevelMark,fld_param_table[iRow].cMaterial));
					lineText.Append(CXhChar50(",\"%s\"",fld_param_table[iRow].codeName));
					lineText.Append(",");lineText.Append(CXhChar16(fld_param_table[iRow].A));
					lineText.Append(",");lineText.Append(CXhChar16(fld_param_table[iRow].B));
					lineText.Append(",");lineText.Append(CXhChar16(fld_param_table[iRow].D));
					lineText.Append(",");lineText.Append(CXhChar16(fld_param_table[iRow].K));
					lineText.Append(",");lineText.Append(CXhChar16(fld_param_table[iRow].L));
					lineText.Append(CXhChar50(",%d,%d",fld_param_table[iRow].nBoltNum,fld_param_table[iRow].nBoltD));
					lineText.Append(",");lineText.Append(CXhChar16(fld_param_table[iRow].C));
					lineText.Append(",");lineText.Append(CXhChar16(fld_param_table[iRow].H));
					lineText.Append(",");lineText.Append(CXhChar16(fld_param_table[iRow].N));
					lineText.Append(",");lineText.Append(CXhChar16(fld_param_table[iRow].S));
					lineText.Append(",");lineText.Append(CXhChar16(fld_param_table[iRow].H1));
					lineText.Append(",");lineText.Append(CXhChar16(fld_param_table[iRow].R));
					for(int i=0;i<5;i++)
					{
						if(fld_param_table[iRow].group[i].t==0&&i>0)
							lineText.Append(",0,0");
						else
							lineText.Append(CXhChar16(",%4.2f,%d",fld_param_table[iRow].group[0].weight,fld_param_table[iRow].group[i].t));
					}
					lineText.Append("},\n");
					fputs(lineText,fp);
				}
				fclose(fp);
				return TRUE;
			}
			fprintf(fp,"%d \n",1003);	//写入版本号 兼容以前定义的法兰库  wht 09-09-14
			fprintf(fp,"%d \n",fld_param_N);
			for(iRow=0;iRow<fld_param_N;iRow++)
			{	
				sprintf(line_txt,"%s\n",fld_param_table[iRow].codeName);	//法兰代号单独写一行，方便处理
				fputs(line_txt,fp);
				//保存法兰重量	wht 10-01-19
				_snprintf(line_txt,499,"%s %s %s %c %c %s %s %s %s %s %d %d %s %s %s %s %s %s",
					(char*)CXhChar16(fld_param_table[iRow].d),
					(char*)CXhChar16(fld_param_table[iRow].couple_d),
					(char*)CXhChar16(fld_param_table[iRow].t),
					fld_param_table[iRow].cLevelMark,
					fld_param_table[iRow].cMaterial,
					(char*)CXhChar16(fld_param_table[iRow].A),
					(char*)CXhChar16(fld_param_table[iRow].B),
					(char*)CXhChar16(fld_param_table[iRow].D),
					(char*)CXhChar16(fld_param_table[iRow].K),
					(char*)CXhChar16(fld_param_table[iRow].L),
					fld_param_table[iRow].nBoltNum,
					fld_param_table[iRow].nBoltD,
					(char*)CXhChar16(fld_param_table[iRow].C),
					(char*)CXhChar16(fld_param_table[iRow].H),
					(char*)CXhChar16(fld_param_table[iRow].N),
					(char*)CXhChar16(fld_param_table[iRow].S),
					(char*)CXhChar16(fld_param_table[iRow].R),
					(char*)CXhChar16(fld_param_table[iRow].H1));
				fputs(line_txt,fp);
				for(int i=0;i<5;i++)
				{
					if(fld_param_table[iRow].group[i].t==0&&i>0)
						break;
					fprintf(fp," %d %s",fld_param_table[iRow].group[i].t,(char*)CXhChar16(fld_param_table[iRow].group[i].weight));
				}
				fprintf(fp,"\n");
			}

		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open welding flange's specification database,the database will be default.";
#else 
				throw "打不开对焊法兰规格库文件,对焊法兰规格库将缺省";
#endif
			int str_len=0,nVersion=0;
			fgets(line_txt,500,fp);
			sscanf(line_txt,"%d",&nVersion);	//尝试读取版本号，新版本第一行存储版本号，旧版本第一行即为法兰库记录个数
			if(nVersion<1001)
				fld_param_N=nVersion;
			else if(nVersion>=1001)
			{	//版本号为1.0.0.1时下一行存取 法兰库记录个数，否则第一行即为法兰库记录个数
				fgets(line_txt,500,fp);
				sscanf(line_txt,"%d",&fld_param_N);
			}
			if(fld_param_N>500)
			{
				fclose(fp);
#ifdef AFX_TARG_ENU_ENGLISH
				throw "System can only handle 200 specification of welding flange at the same time";
#else 
				throw "系统只能同时处理500种对焊法兰规格";
#endif
			}
			//清空原有数据
			for(iRow=0;iRow<500;iRow++)
				memset(&fld_param_table[iRow],0,sizeof(FL_PARAM));
			//读取新的数据
			for(iRow=0;iRow<fld_param_N;iRow++)
			{
				if(nVersion>=1001)
				{	//1.0.0.1
					fgets(line_txt,16,fp);
					sscanf(line_txt,"%s",fld_param_table[iRow].codeName);
					str_len=strlen(fld_param_table[iRow].codeName);
					if(fld_param_table[iRow].codeName[str_len-1]=='\n')
						fld_param_table[iRow].codeName[str_len-1]='\0';
				}
				fgets(line_txt,500,fp);
				if(nVersion==1002)
				{
					sscanf(line_txt,"%lf %lf %c %c %lf %lf %lf %lf %lf %d %d %lf %lf %lf %lf %lf %lf %d %lf %d %lf %d %lf %d %lf %d %lf",
						&fld_param_table[iRow].d,&fld_param_table[iRow].t,&fld_param_table[iRow].cLevelMark,
						&fld_param_table[iRow].cMaterial,&fld_param_table[iRow].A,&fld_param_table[iRow].B,
						&fld_param_table[iRow].D,&fld_param_table[iRow].K,&fld_param_table[iRow].L,&fld_param_table[iRow].nBoltNum,
						&fld_param_table[iRow].nBoltD,&fld_param_table[iRow].C,&fld_param_table[iRow].H,&fld_param_table[iRow].N,
						&fld_param_table[iRow].S,&fld_param_table[iRow].R,&fld_param_table[iRow].H1,
						&fld_param_table[iRow].group[0].t,&fld_param_table[iRow].group[0].weight,
						&fld_param_table[iRow].group[1].t,&fld_param_table[iRow].group[1].weight,
						&fld_param_table[iRow].group[2].t,&fld_param_table[iRow].group[2].weight,
						&fld_param_table[iRow].group[3].t,&fld_param_table[iRow].group[3].weight,
						&fld_param_table[iRow].group[4].t,&fld_param_table[iRow].group[4].weight);
				}
				else if(nVersion==1003)
				{	//增加读取对接钢管直径
					sscanf(line_txt,"%lf %lf %lf %c %c %lf %lf %lf %lf %lf %d %d %lf %lf %lf %lf %lf %lf %d %lf %d %lf %d %lf %d %lf %d %lf",
						&fld_param_table[iRow].d,
						&fld_param_table[iRow].couple_d,
						&fld_param_table[iRow].t,
						&fld_param_table[iRow].cLevelMark,
						&fld_param_table[iRow].cMaterial,
						&fld_param_table[iRow].A,
						&fld_param_table[iRow].B,
						&fld_param_table[iRow].D,
						&fld_param_table[iRow].K,
						&fld_param_table[iRow].L,
						&fld_param_table[iRow].nBoltNum,
						&fld_param_table[iRow].nBoltD,
						&fld_param_table[iRow].C,
						&fld_param_table[iRow].H,
						&fld_param_table[iRow].N,
						&fld_param_table[iRow].S,
						&fld_param_table[iRow].R,
						&fld_param_table[iRow].H1,
						&fld_param_table[iRow].group[0].t,&fld_param_table[iRow].group[0].weight,
						&fld_param_table[iRow].group[1].t,&fld_param_table[iRow].group[1].weight,
						&fld_param_table[iRow].group[2].t,&fld_param_table[iRow].group[2].weight,
						&fld_param_table[iRow].group[3].t,&fld_param_table[iRow].group[3].weight,
						&fld_param_table[iRow].group[4].t,&fld_param_table[iRow].group[4].weight);
				}
				else
				{
					sscanf(line_txt,"%lf %lf %c %c %lf %lf %lf %lf %lf %d %d %lf %lf %lf %lf %lf %lf %lf",
						&fld_param_table[iRow].d,&fld_param_table[iRow].t,&fld_param_table[iRow].cLevelMark,
						&fld_param_table[iRow].cMaterial,&fld_param_table[iRow].A,&fld_param_table[iRow].B,
						&fld_param_table[iRow].D,&fld_param_table[iRow].K,&fld_param_table[iRow].L,&fld_param_table[iRow].nBoltNum,
						&fld_param_table[iRow].nBoltD,&fld_param_table[iRow].C,&fld_param_table[iRow].H,&fld_param_table[iRow].N,
						&fld_param_table[iRow].S,&fld_param_table[iRow].R,&fld_param_table[iRow].H1,&fld_param_table[iRow].group[0].weight);
				}
				if(str_len==0||nVersion<1001)	//! 1.0.0.1
					sprintf(fld_param_table[iRow].codeName,"FLD%.f%c",fld_param_table[iRow].d,
						fld_param_table[iRow].cLevelMark);
			}
		}
		fclose(fp);
	}
	catch(char* sError)
	{
		if(bPrompt)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}

BOOL FlPSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt)
{
	FILE *fp;
	char file_path[MAX_PATH],line_txt[500];
	sprintf(file_path,"%s",sFileName);
	if(strlen(file_path)<=0)
		return FALSE;
	int iRow=0;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open weld flange's specification database,the database remain unchanged. ";
#else 
				throw "打不开平焊法兰规格库文件,平焊法兰规格库保持不变";
#endif
			bool bCodeMode=false;
			if(bCodeMode)
			{	//仅用于更析代码中数据段用
				fprintf(fp,"%d \n",flp_param_N);
				CXhChar500 lineText;
				for(iRow=0;iRow<flp_param_N;iRow++)
				{	
					lineText.Printf("{%s,%s",(char*)CXhChar16(flp_param_table[iRow].d),(char*)CXhChar16(flp_param_table[iRow].t));
					lineText.Append(CXhChar16(",'%C','%C'",flp_param_table[iRow].cLevelMark,flp_param_table[iRow].cMaterial));
					lineText.Append(CXhChar50(",\"%s\"",flp_param_table[iRow].codeName));
					lineText.Append(",");lineText.Append(CXhChar16(flp_param_table[iRow].A));
					lineText.Append(",");lineText.Append(CXhChar16(flp_param_table[iRow].B));
					lineText.Append(",");lineText.Append(CXhChar16(flp_param_table[iRow].D));
					lineText.Append(",");lineText.Append(CXhChar16(flp_param_table[iRow].K));
					lineText.Append(",");lineText.Append(CXhChar16(flp_param_table[iRow].L));
					lineText.Append(CXhChar50(",%d,%d",flp_param_table[iRow].nBoltNum,flp_param_table[iRow].nBoltD));
					lineText.Append(",");lineText.Append(CXhChar16(flp_param_table[iRow].C));
					lineText.Append(",");lineText.Append(CXhChar16(flp_param_table[iRow].H));
					lineText.Append(",");lineText.Append(CXhChar16(flp_param_table[iRow].N));
					lineText.Append(",");lineText.Append(CXhChar16(flp_param_table[iRow].S));
					lineText.Append(",");lineText.Append(CXhChar16(flp_param_table[iRow].H1));
					lineText.Append(",");lineText.Append(CXhChar16(flp_param_table[iRow].R));
					for(int i=0;i<5;i++)
					{
						if(flp_param_table[iRow].group[i].t==0&&i>0)
							lineText.Append(",0,0");
						else
							lineText.Append(CXhChar16(",%4.2f,%d",flp_param_table[iRow].group[0].weight,flp_param_table[iRow].group[i].t));
					}
					lineText.Append("},\n");
					fputs(lineText,fp);
				}
				fclose(fp);
				return TRUE;
			}
			fprintf(fp,"%d \n",1001);	//写入版本号 兼容以前定义的法兰库  wht 09-09-14
			fprintf(fp,"%d \n",flp_param_N);
			for(iRow=0;iRow<flp_param_N;iRow++)
			{	
				sprintf(line_txt,"%s\n",flp_param_table[iRow].codeName);	//法兰代号单独写一行，方便处理
				fputs(line_txt,fp);
				//保存法兰重量	wht 10-01-19
				sprintf(line_txt,"%s %s %c %c %s %s %s %s %s %d %d %s %s %s %s %s %s\n",
					(char*)CXhChar16(flp_param_table[iRow].d),(char*)CXhChar16(flp_param_table[iRow].t),flp_param_table[iRow].cLevelMark,
					flp_param_table[iRow].cMaterial,(char*)CXhChar16(flp_param_table[iRow].A),(char*)CXhChar16(flp_param_table[iRow].B),
					(char*)CXhChar16(flp_param_table[iRow].D),(char*)CXhChar16(flp_param_table[iRow].K),(char*)CXhChar16(flp_param_table[iRow].L),
					flp_param_table[iRow].nBoltNum,flp_param_table[iRow].nBoltD,(char*)CXhChar16(flp_param_table[iRow].C),
					(char*)CXhChar16(flp_param_table[iRow].H),(char*)CXhChar16(flp_param_table[iRow].N),(char*)CXhChar16(flp_param_table[iRow].S),
					(char*)CXhChar16(flp_param_table[iRow].R),(char*)CXhChar16(flp_param_table[iRow].H1),
					(char*)CXhChar16(flp_param_table[iRow].group[0].weight));
				fputs(line_txt,fp);
			}
		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open welding flange's specification database,the database will be default.";
#else 
				throw "打不开平焊法兰规格库文件,平焊法兰规格库将缺省";
#endif
			int str_len=0,nVersion=0;
			fscanf(fp,"%d",&nVersion);	//尝试读取版本号，新版本第一行存储版本号，旧版本第一行即为法兰库记录个数
			fgets(line_txt,500,fp);
			if(nVersion==1001)
			{	//版本号为1.0.0.1时下一行存取 法兰库记录个数，否则第一行即为法兰库记录个数
				fscanf(fp,"%d",&flp_param_N);
				fgets(line_txt,500,fp);
			}
			else
				flp_param_N=nVersion;
			if(flp_param_N>200)
			{
				fclose(fp);
#ifdef AFX_TARG_ENU_ENGLISH
				throw "System can only handle 200 specifications of welding flange at the same time.";
#else 
				throw "系统只能同时处理200种平焊法兰规格";
#endif
			}
			for(iRow=0;iRow<flp_param_N;iRow++)
			{
				if(nVersion==1001)
				{	//1.0.0.1
					fgets(line_txt,16,fp);
					sscanf(line_txt,"%s",flp_param_table[iRow].codeName);
					str_len=strlen(flp_param_table[iRow].codeName);
					if(flp_param_table[iRow].codeName[str_len-1]=='\n')
						flp_param_table[iRow].codeName[str_len-1]='\0';
				}
				fgets(line_txt,500,fp);
				sscanf(line_txt,"%lf %lf %c %c %lf %lf %lf %lf %lf %d %d %lf %lf %lf %lf %lf %lf %lf",
					&flp_param_table[iRow].d,&flp_param_table[iRow].t,&flp_param_table[iRow].cLevelMark,
					&flp_param_table[iRow].cMaterial,&flp_param_table[iRow].A,&flp_param_table[iRow].B,
					&flp_param_table[iRow].D,&flp_param_table[iRow].K,&flp_param_table[iRow].L,&flp_param_table[iRow].nBoltNum,
					&flp_param_table[iRow].nBoltD,&flp_param_table[iRow].C,&flp_param_table[iRow].H,&flp_param_table[iRow].N,
					&flp_param_table[iRow].S,&flp_param_table[iRow].R,&flp_param_table[iRow].H1,&flp_param_table[iRow].group[0].weight);
				if(str_len==0||nVersion!=1001)	//! 1.0.0.1
					sprintf(flp_param_table[iRow].codeName,"FLP%.f%c",flp_param_table[iRow].d,
						flp_param_table[iRow].cLevelMark);
			}
		}
		fclose(fp);
	}
	catch(char* sError)
	{
		if(bPrompt)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}
BOOL FlGSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt)
{
	FILE *fp;
	char file_path[MAX_PATH],line_txt[500];
	sprintf(file_path,"%s",sFileName);
	if(strlen(file_path)<=0)
		return FALSE;
	int iRow=0;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open rigid flange's specification database ,the database remains unchanged";
#else 
				throw "打不开刚性法兰规格库文件,刚性法兰规格库保持不变";
#endif
			fprintf(fp,"%d \n",flg_param_N);
			for(iRow=0;iRow<flg_param_N;iRow++)
			{	
				sprintf(line_txt,"%s\n",flg_param_tbl[iRow].codeName);	//法兰代号单独写一行，方便处理
				fputs(line_txt,fp);
				sprintf(line_txt,"%s %s %c %c %s %s %s %s %d %d %s %s %s %d %s %c %s %s %s %s %s %s %s %s %s\n",
					(char*)CXhChar16(flg_param_tbl[iRow].d),(char*)CXhChar16(flg_param_tbl[iRow].couple_d),flg_param_tbl[iRow].cLevelMark,
					flg_param_tbl[iRow].cMaterial,(char*)CXhChar16(flg_param_tbl[iRow].fInnerD),(char*)CXhChar16(flg_param_tbl[iRow].fOuterD),
					(char*)CXhChar16(flg_param_tbl[iRow].fBoltCirD),(char*)CXhChar16(flg_param_tbl[iRow].fBoltHoleD),
					flg_param_tbl[iRow].nBoltNum,flg_param_tbl[iRow].nBoltD,(char*)CXhChar16(flg_param_tbl[iRow].fThick),
					(char*)CXhChar16(flg_param_tbl[iRow].fH),(char*)CXhChar16(flg_param_tbl[iRow].theroy_weight),flg_param_tbl[iRow].nRibPlateNum,
					(char*)CXhChar16(flg_param_tbl[iRow].para.weight),flg_param_tbl[iRow].para.cMat,(char*)CXhChar16(flg_param_tbl[iRow].para.T),
					(char*)CXhChar16(flg_param_tbl[iRow].para.A),(char*)CXhChar16(flg_param_tbl[iRow].para.B),(char*)CXhChar16(flg_param_tbl[iRow].para.C),
					(char*)CXhChar16(flg_param_tbl[iRow].para.D),(char*)CXhChar16(flg_param_tbl[iRow].para.E),(char*)CXhChar16(flg_param_tbl[iRow].para.F),
					(char*)CXhChar16(flg_param_tbl[iRow].para.G),(char*)CXhChar16(flg_param_tbl[iRow].para.H));
					fputs(line_txt,fp);
			}
		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open rigid flange's specification database ,the database will be default";
#else 
				throw "打不开刚性法兰规格库文件,刚性法兰规格库将缺省";
#endif
			fscanf(fp,"%d",&flg_param_N);
			fgets(line_txt,500,fp);
			if(flg_param_N>200)
			{
				fclose(fp);
#ifdef AFX_TARG_ENU_ENGLISH
				throw "System can only handle 200 specification of rigid flange at the same time.";
#else 
				throw "系统只能同时处理200种刚性法兰规格";
#endif
			}
			for(iRow=0;iRow<flg_param_N;iRow++)
			{
				fgets(line_txt,16,fp);
				sscanf(line_txt,"%s",flg_param_tbl[iRow].codeName);
				fgets(line_txt,500,fp);
				sscanf(line_txt,"%lf %lf %c %c %lf %lf %lf %lf %d %d %lf %lf %lf %d %lf %c %lf %lf %lf %lf %lf %lf %lf %lf %lf",
					&flg_param_tbl[iRow].d,&flg_param_tbl[iRow].couple_d,&flg_param_tbl[iRow].cLevelMark,
					&flg_param_tbl[iRow].cMaterial,&flg_param_tbl[iRow].fInnerD,&flg_param_tbl[iRow].fOuterD,
					&flg_param_tbl[iRow].fBoltCirD,&flg_param_tbl[iRow].fBoltHoleD,&flg_param_tbl[iRow].nBoltNum,
					&flg_param_tbl[iRow].nBoltD,&flg_param_tbl[iRow].fThick,&flg_param_tbl[iRow].fH,
					&flg_param_tbl[iRow].theroy_weight,&flg_param_tbl[iRow].nRibPlateNum,
					&flg_param_tbl[iRow].para.weight,&flg_param_tbl[iRow].para.cMat,&flg_param_tbl[iRow].para.T,
					&flg_param_tbl[iRow].para.A,&flg_param_tbl[iRow].para.B,&flg_param_tbl[iRow].para.C,
					&flg_param_tbl[iRow].para.D,&flg_param_tbl[iRow].para.E,&flg_param_tbl[iRow].para.F,
					&flg_param_tbl[iRow].para.G,&flg_param_tbl[iRow].para.H);
			}
		}
		fclose(fp);
	}
	catch(char* sError)
	{
		if(bPrompt)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}
BOOL FlRSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt)
{
	FILE *fp;
	char file_path[MAX_PATH],line_txt[500];
	sprintf(file_path,"%s",sFileName);
	if(strlen(file_path)<=0)
		return FALSE;
	int iRow=0;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open flexible flange's database,the database remains unchanged.";
#else 
				throw "打不开柔性法兰规格库文件,柔性法兰规格库保持不变";
#endif
			fprintf(fp,"%d \n",flr_param_N);
			for(iRow=0;iRow<flr_param_N;iRow++)
			{
				sprintf(line_txt,"%s\n",flr_param_tbl[iRow].codeName);	//法兰代号单独写一行，方便处理
				fputs(line_txt,fp);
				sprintf(line_txt,"%s %s %c %c %s %s %s %s %d %d %s %s %s %d %s %c %s %s %s %s %s %s %s %s %s\n",
					(char*)CXhChar16(flr_param_tbl[iRow].d),(char*)CXhChar16(flr_param_tbl[iRow].couple_d),flr_param_tbl[iRow].cLevelMark,
					flr_param_tbl[iRow].cMaterial,(char*)CXhChar16(flr_param_tbl[iRow].fInnerD),(char*)CXhChar16(flr_param_tbl[iRow].fOuterD),
					(char*)CXhChar16(flr_param_tbl[iRow].fBoltCirD),(char*)CXhChar16(flr_param_tbl[iRow].fBoltHoleD),
					flr_param_tbl[iRow].nBoltNum,flr_param_tbl[iRow].nBoltD,(char*)CXhChar16(flr_param_tbl[iRow].fThick),
					(char*)CXhChar16(flr_param_tbl[iRow].fH),(char*)CXhChar16(flr_param_tbl[iRow].theroy_weight),flr_param_tbl[iRow].nRibPlateNum,
					(char*)CXhChar16(flr_param_tbl[iRow].para.weight),flr_param_tbl[iRow].para.cMat,(char*)CXhChar16(flr_param_tbl[iRow].para.T),
					(char*)CXhChar16(flr_param_tbl[iRow].para.A),(char*)CXhChar16(flr_param_tbl[iRow].para.B),(char*)CXhChar16(flr_param_tbl[iRow].para.C),
					(char*)CXhChar16(flr_param_tbl[iRow].para.D),(char*)CXhChar16(flr_param_tbl[iRow].para.E),(char*)CXhChar16(flr_param_tbl[iRow].para.F),
					(char*)CXhChar16(flr_param_tbl[iRow].para.G),(char*)CXhChar16(flr_param_tbl[iRow].para.H));
				fputs(line_txt,fp);
			}
		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open flexible flange's database,the database will be default.";
#else 
				throw "打不开柔性法兰规格库文件,柔性法兰规格库将缺省";
#endif
			fscanf(fp,"%d",&flr_param_N);
			fgets(line_txt,500,fp);
			if(flr_param_N>200)
			{
				fclose(fp);
#ifdef AFX_TARG_ENU_ENGLISH
				throw "System can only handle 200 specification of flexible flange at the same time.";
#else 
				throw "系统只能同时处理200种柔性法兰规则";
#endif
			}
			for(iRow=0;iRow<flr_param_N;iRow++)
			{
				fgets(line_txt,16,fp);
				sscanf(line_txt,"%s",flr_param_tbl[iRow].codeName);
				fgets(line_txt,500,fp);
				sscanf(line_txt,"%lf %lf %c %c %lf %lf %lf %lf %d %d %lf %lf %lf %d %lf %c %lf %lf %lf %lf %lf %lf %lf %lf %lf",
					&flr_param_tbl[iRow].d,&flr_param_tbl[iRow].couple_d,&flr_param_tbl[iRow].cLevelMark,
					&flr_param_tbl[iRow].cMaterial,&flr_param_tbl[iRow].fInnerD,&flr_param_tbl[iRow].fOuterD,
					&flr_param_tbl[iRow].fBoltCirD,&flr_param_tbl[iRow].fBoltHoleD,&flr_param_tbl[iRow].nBoltNum,
					&flr_param_tbl[iRow].nBoltD,&flr_param_tbl[iRow].fThick,&flr_param_tbl[iRow].fH,
					&flr_param_tbl[iRow].theroy_weight,&flr_param_tbl[iRow].nRibPlateNum,
					&flr_param_tbl[iRow].para.weight,&flr_param_tbl[iRow].para.cMat,&flr_param_tbl[iRow].para.T,
					&flr_param_tbl[iRow].para.A,&flr_param_tbl[iRow].para.B,&flr_param_tbl[iRow].para.C,
					&flr_param_tbl[iRow].para.D,&flr_param_tbl[iRow].para.E,&flr_param_tbl[iRow].para.F,
					&flr_param_tbl[iRow].para.G,&flr_param_tbl[iRow].para.H);
			}
		}
		fclose(fp);
	}
	catch(char* sError)
	{
		if(bPrompt)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}
BOOL FlCoupleLevelSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt)
{
	FILE *fp;
	char file_path[MAX_PATH],line_txt[500];
	sprintf(file_path,"%s",sFileName);
	if(strlen(file_path)<=0)
		return FALSE;
	int iRow=0;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open flange's butting file,the file remains unchanged.";
#else 
				throw "打不开法兰对接级别定义文件,法兰对接级别定义保持不变";
#endif
			bool bCodeMode=false;
			if(bCodeMode)
			{	//仅用于更析代码中数据段用
				fprintf(fp,"%d \n",fl_couple_rule_N);
				CXhChar500 lineText;
				for(iRow=0;iRow<fl_couple_rule_N;iRow++)
				{	
					lineText.Printf("	{%3.0f,%3.0f,%3.0f,%3.0f,%3.0f},",fl_couple_rule_table[iRow].d,
						fl_couple_rule_table[iRow].A,fl_couple_rule_table[iRow].B,
						fl_couple_rule_table[iRow].C,fl_couple_rule_table[iRow].D);
					lineText.Append("\n");
					fputs(lineText,fp);
				}
				fclose(fp);
				return TRUE;
			}
			fprintf(fp,"%d \n",fl_couple_rule_N);
			for(iRow=0;iRow<fl_couple_rule_N;iRow++)
			{	//主管直径d		A	B	C	D 
				sprintf(line_txt,"%lf %lf %lf %lf %lf\n",
					fl_couple_rule_table[iRow].d,fl_couple_rule_table[iRow].A,fl_couple_rule_table[iRow].B,
					fl_couple_rule_table[iRow].C,fl_couple_rule_table[iRow].D);
				fputs(line_txt,fp);
			}
		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
#ifdef AFX_TARG_ENU_ENGLISH
				throw "Can't open flange's butting file,the file will be default.";
#else 
				throw "打不开法兰对接级别定义文件,法兰对接定义规则将缺省";
#endif
			fscanf(fp,"%d",&fl_couple_rule_N);
			fgets(line_txt,500,fp);
			if(fl_couple_rule_N>200)
			{
				fclose(fp);
#ifdef AFX_TARG_ENU_ENGLISH
				throw "System can only handle 200 butting specification of flange.";
#else 
				throw "系统只能同时处理200种法兰对接定义规则";
#endif
			}
			for(iRow=0;iRow<fl_couple_rule_N;iRow++)
			{
				fgets(line_txt,500,fp);
				sscanf(line_txt,"%lf %lf %lf %lf %lf",
					&fl_couple_rule_table[iRow].d,&fl_couple_rule_table[iRow].A,&fl_couple_rule_table[iRow].B,
					&fl_couple_rule_table[iRow].C,&fl_couple_rule_table[iRow].D);
			}
		}
		fclose(fp);
	}
	catch(char* sError)
	{
		if(bPrompt)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}

BOOL AidedPartGuiGeSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt)
{
	FILE *fp;
	char file_path[MAX_PATH];
	sprintf(file_path,"%s",sFileName);
	if(strlen(file_path)<=0)
		return FALSE;
	int iRow;
	try
	{
		if(bWrite)	//导出
		{
			if((fp=fopen(file_path,"wt"))==NULL)
				return FALSE;//throw CString("打不开辅助构件规格库文件,辅助构件规格库保持不变");
			fprintf(fp,"%d \n",aided_part_N);
			for(iRow=0;iRow<aided_part_N;iRow++)
			{
				fprintf(fp,"%s %8.3f %d",aided_part_table[iRow].guige,aided_part_table[iRow].weight,aided_part_table[iRow].siUnitType);
				fprintf(fp,"\n");
			}
		}
		else		//导入
		{
			if((fp=fopen(file_path,"rt"))==NULL)
				return FALSE;//throw CString("打不开辅助构件规格库文件,辅助构件规格库将缺省");
			fscanf(fp,"%d",&aided_part_N);
			if(aided_part_N>100)
			{
				fclose(fp);
				throw CString("系统只能同时处理100种扁铁规格");
			}
			for(iRow=0;iRow<aided_part_N;iRow++)
			{
				fscanf(fp,"%s %lf %d",&aided_part_table[iRow].guige,&aided_part_table[iRow].weight,&aided_part_table[iRow].siUnitType);
			}
		}
		fclose(fp);
	}
	catch(char* sError)
	{
		if(bPrompt)
			AfxMessageBox(sError);
		return FALSE;
	}
	return TRUE;
}