#include "iztools.h"

std::map<std::string, int> m_p, m_z;
void InitTextMap() {
    m_p["空格"] = m_p["."] = m_p[" "] = 0xff;
    m_p["单发"] = m_p["豌豆"] = m_p["1"] = 0;
    m_p["小向"] = m_p["h"] = 1;
    m_p["坚果"] = m_p["o"] = 3;
    m_p["地雷"] = m_p["土豆"] = m_p["t"] = 4;
    m_p["冰豆"] = m_p["寒冰"] = m_p["雪花"] = m_p["雪豆"] = m_p["b"] = 5;
    m_p["大嘴"] = m_p["z"] = 6;
    m_p["双发"] = m_p["2"] = 7;
    m_p["小喷"] = m_p["p"] = 8;
    m_p["大喷"] = m_p["d"] = 10;
    m_p["胆小"] = m_p["x"] = 13;
    m_p["倭瓜"] = m_p["窝瓜"] = m_p["w"] = 17;
    m_p["三线"] = m_p["3"] = 18;
    m_p["地刺"] = m_p["棘草"] = m_p["_"] = 21;
    m_p["火树"] = m_p["火炬"] = m_p["树桩"] = m_p["j"] = 22;
    m_p["分裂"] = m_p["裂荚"] = m_p["双向"] = m_p["l"] = 28;
    m_p["杨桃"] = m_p["星星"] = m_p["5"] = 29;
    m_p["磁铁"] = m_p["c"] = 31;
    m_p["玉米"] = m_p["黄油"] = m_p["y"] = 34;
    m_p["伞叶"] = m_p["叶子"] = m_p["s"] = 37;

    m_z["空格"] = 0;
    m_z["小鬼"] = m_z["xg"] = 1;
    m_z["路障"] = m_z["lz"] = 2;
    m_z["撑杆"] = m_z["cg"] = 3;
    m_z["铁桶"] = m_z["tt"] = 4;
    m_z["蹦极"] = m_z["小偷"] = m_z["bj"] = m_z["xt"] = 5;
    m_z["矿工"] = m_z["kg"] = 6;
    m_z["梯子"] = m_z["扶梯"] = m_z["tz"] = m_z["ft"] = 8;
    m_z["橄榄"] = m_z["gl"] = 9;
    m_z["舞王"] = m_z["ww"] = m_z["mj"] = 10;
}
int FindPlantInMap(std::string s) {
    int t = 0;
    while((t = s.find(' ')) != -1)
        s.erase(t, 1);
    if(m_p.count(s) == 0) return -1;
    return m_p[s];
}
int FindZombieInMap(std::string s) {
    int t = 0;
    while((t = s.find(' ')) != -1)
        s.erase(t, t);
    if(m_z.count(s) == 0) return -1;
    return m_z[s];
}

const char* str_help = R"RAWSTRING(【植物类型】使用全称或符号记法（详见符号对照表）
【植物编号】即常说的“栈位”，决定了多个植物同时判定时的优先顺序
【僵尸类型】使用全称或拼音缩写（如“xg”）
【僵尸时刻】单位为帧（正常速度下0.01秒），留空默认0
【僵尸列】 留空默认6列
【目标列】吃掉该列植物视为通过，填0表示大脑
)RAWSTRING";
const char* str_help5 = R"RAWSTRING(【全场测试格式】
> 目标位置：r-c表示植物，r-0表示大脑
> 植物：5行，符号记法，忽略空白字符，符号后面使用加号(+)表示延迟一轮种植
> 僵尸类型（与下面两项对应）
> 僵尸放置时刻：单位为帧（正常速度下0.01秒）
> 僵尸放置位置：r-c
【舞王锁定】
填写锁定的相位（0~459的整数），确保0时刻相位固定；留空则不锁定，确保0时刻相位随机
)RAWSTRING";
const char* str_char = R"RAWSTRING([.] 空格
[1] 单发/豌豆
[h] 小向
[o] 坚果
[t] 地雷/土豆
[b] 冰豆/寒冰/雪花/雪豆
[z] 大嘴
[2] 双发
[p] 小喷
[d] 大喷
[x] 胆小
[w] 倭瓜/窝瓜
[3] 三线
[_] 地刺/棘草
[j] 火树/火炬/树桩
[l] 分裂/裂荚/双向
[5] 杨桃/星星
[c] 磁铁
[y] 玉米/黄油
[s] 伞叶/叶子)RAWSTRING";
const char* str_about =
    "--- IZ测试工具 ---\n\
版本号：" VERSION
    "\n\
作者：sqrt_7\n\
GitHub: sqrt07/iztools\n\
本工具仅适用于PVZ英文原版1.0.0.1051，中文版无法使用\
";

const char* str_5test =
    "\
3-0 4-0 5-0 3-3\r\n\
.....\r\n\
.....\r\n\
bs3_c\r\n\
b2ljh\r\n\
blyl_\r\n\
cg   cg   xg   ww\r\n\
0    1    300  700\r\n\
4-6  4-6  4-6  4-6";

const char* str_record =
    "\
提示：重开本关（RESTART）以开始录制；再次重开以自动保存录像。\n\
按方向键↑加速，方向键↓减速。";

const char* str_replay =
    "\
提示：重开本关（RESTART）以开始回放；再次重开或关闭本窗口以停止回放。回放时请勿操作游戏。\n\
按方向键↑加速，方向键↓减速。";

const char* str_1400warning = 
    "\
在录制冲关时，自动修改开局阵型为最优情况。\n\
由于某些历史原因，部分玩家在冲关时使用固定的1400阳光开局。\n\
不建议使用，您是否一定要开启？\n";