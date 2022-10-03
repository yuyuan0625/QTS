#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#define expTime 50
#define ITERATION 5000
#define POPULATION 100
#define DELTA 0.002
#define nmax 4  //max n
#define mmax 30 // max m
#define fun_num  16
#define STATE 4 //0~1 divide to 4 states
using namespace std;

ofstream file("QTS_fixed_bug.txt");
double Q[nmax][mmax][STATE] = {0}; //4個狀態(0: 0 control;1: 1 control; 2:wire; 3:not gate)
int X[POPULATION][nmax][mmax] = {0};
int gb[nmax][mmax] = {0}, gw[nmax][mmax] = {0};
double gb_fit = 0, gw_fit = 100;
double fit[POPULATION] = {0.0};
int n = 0;
int m = 0;

int function[fun_num][32]{
    {1, 0, 3, 2, 5, 7, 4, 6},                               // 0
    {7, 0, 1, 2, 3, 4, 5, 6},                               // 1
    {0, 1, 2, 3, 4, 6, 5, 7},                               // 2
    {0, 1, 2, 4, 3, 5, 6, 7},                               // 3
    {0, 1, 14, 15, 4, 5, 10, 11, 7, 9, 6, 8, 12, 13, 2, 3}, // 4
    {1, 2, 3, 4, 5, 6, 7, 0},                               // 5
    {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0}, // 6
    {0, 7, 6, 9, 4, 11, 10, 13, 8, 15, 14, 1, 12, 3, 2, 5}, // 7
    {3, 6, 2, 5, 7, 1, 0, 4},                               // 8
    {1, 2, 7, 5, 6, 3, 0, 4},                               // 9
    {4, 3, 0, 2, 7, 5, 6, 1},                               // 10
    {7, 5, 4, 2, 0, 1, 6, 3},                               // 11
    {6, 3, 14, 13, 2, 11, 7, 10, 0, 5, 8, 1, 12, 15, 9, 4}, // 12
    {0, 9, 10, 5, 4, 15, 14, 8, 11, 2, 6, 3, 12, 7, 1, 13}, // 13
    {6, 4, 11, 0, 9, 8, 12, 2, 15, 5, 3, 7, 10, 13, 14, 1}, // 14
    {13, 1, 14, 0, 9, 2, 15, 6, 12, 8, 11, 3, 4, 5, 7, 10}  // 15
};

int n_arr[] = {3, 3, 3, 3, 4, 3, 4, 4, 3, 3, 3, 3, 4, 4, 4, 4}; //n of 16 function
int m_arr[] = {6, 6, 6, 10, 17, 6, 6, 8, 8, 10, 8, 8, 13, 17, 16, 16};  //m of 16 function
int output[32];

void init() //init Q
{
    gb_fit = 0.0;
    gw_fit = 100;
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < m; j++)
        {
            for (int k = 0; k < 4; k++)
            {
                Q[i][j][k] = 0.25;
            }
        }
    }
}

void measure() //產生n*m個random r ,看介於哪個區間->轉換成對應gate
{
    for (int i = 0; i < POPULATION; i++)
    {
        for (int j = 0; j < n; j++)
        {
            for (int k = 0; k < m; k++)
            {
                double r = (double)rand() / RAND_MAX; //r:0~1
                if (0 <= r && r <= Q[j][k][0])
                {
                    X[i][j][k] = 0;
                }
                else if (Q[j][k][0] < r && r <= (Q[j][k][0] + Q[j][k][1]))
                {
                    X[i][j][k] = 1;
                }
                else if ((Q[j][k][0] + Q[j][k][1]) < r && r <= (Q[j][k][0] + Q[j][k][1] + Q[j][k][2]))
                {
                    X[i][j][k] = 2;
                }
                else
                    X[i][j][k] = 3;
            }
        }
    }
}

void repair() //一行不能有多個NOT(3)
{
    for (int i = 0; i < POPULATION; i++)
    {
        for (int j = 0; j < m; j++) //gate
        {
            int max_notProb_index = -1; //max not prob gate
            int target_cnt = 0;
            double max_notProb = 0.0;
            for (int k = 0; k < n; k++) //input number
            {
                if (X[i][k][j] == 3)
                {
                    if (target_cnt == 0) //第一個not
                    {
                        max_notProb_index = k;
                        max_notProb = Q[k][j][3];
                        target_cnt++;
                    }

                    else //多個not->兩個相比,更新notProb較低者
                    {
                        if (Q[k][j][3] > max_notProb) //更新舊的
                        {
                            int max_elseProb_index = -1;
                            double max_elseProb = 0.0;
                            for (int a = 0; a < 3; a++)
                            {
                                if (Q[max_notProb_index][j][a] > max_elseProb)
                                {
                                    max_elseProb = Q[max_notProb_index][j][a];
                                    max_elseProb_index = a;
                                }
                            }
                            X[i][max_notProb_index][j] = max_elseProb_index;
                            max_notProb_index = k;
                            max_notProb = Q[k][j][3];
                        }
                        else //更新自己
                        {
                            int max_elseProb_index = -1;
                            double max_elseProb = 0.0;
                            for (int a = 0; a < 3; a++)
                            {
                                if (Q[k][j][a] > max_elseProb)
                                {
                                    max_elseProb = Q[k][j][a];
                                    max_elseProb_index = a;
                                }
                            }
                            X[i][k][j] = max_elseProb_index;
                        }
                    }
                }
            }
        }
    }
}

void printQ()
{
    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < n; j++)
        {
            for (int k = 0; k < 4; k++)
            {
                cout << Q[j][i][k] << " ";
            }
            cout << endl;
        }
    }
}

void test_answer()
{
    for (int p = 0; p < POPULATION; p++)
    {
        for (int i = 0; i < m; i++)
        {
            for (int j = 0; j < n; j++)
            {
                cout << X[p][j][i] << " ";
            }
            cout << endl;
        }
    }
}

int toDec(int *num)
{
    int decimal = 0, exp = 1;
    for (int i = n - 1; i >= 0; i--) //從最後一位乘，每輪多一次方
    {
        decimal += num[i] * exp;
        exp *= 2;
    }

    return decimal;
} //by hsu

int *toBin(int dec)
{
    int *binary = new int[n];
    for (int i = n - 1; i >= 0; i--)
    {
        binary[i] = dec % 2;
        dec /= 2;
    }
    return binary;
}

bool correct(int index, int in, int out) //有問題
{
    int *bin_in = toBin(in);
    for (int i = 0; i < m; i++)
    {
        int notPos = -1;
        bool changeNot = true;
        for (int j = 0; j < n; j++)
        {
            if ((X[index][j][i] == 0 || X[index][j][i] == 1) && X[index][j][i] != bin_in[j]) //沒有符合0 or 1 control-> not gate值不變
            {
                changeNot = false;
                break;
            }
            else if (X[index][j][i] == 3) //紀錄not gate位置
                notPos = j;
        }
        if (notPos != -1 && changeNot == true)
        {
            bin_in[notPos] = !bin_in[notPos];
        }
    }
    // cout << "Dec: " << toDec(bin_in) << "\t";
    // cout << "out:" << out << endl;
    if (toDec(bin_in) == out)
    {
        delete bin_in;
        return true;
    }

    delete bin_in;
    return false;
    
        
}

int COP(int index)
{
    int cnt = 0;
    for (int i = 0; i < pow(2, n); i++)
    {
        if (correct(index, i, output[i]))
        {
            cnt++;
        }
    }
    return cnt;
}

int WG(int i) //wire gate number
{
    int not_cnt = 0;
    for (int j = 0; j < m; j++)
    {
        for (int k = 0; k < n; k++)
        {
            if (X[i][k][j] == 3)
            {
                not_cnt++;
                break;
            }
        }
    }
    return m - not_cnt;
}

void fitness()
{
    for (int i = 0; i < POPULATION; i++)
    {
        double fit1 = 0.0, fit2 = 0.0;
        double w1 = 1, w2 = 0.0;
        fit1 = COP(i) / (double)pow(2, n);
        if (fit1 == 1)
            w2 = 0.4;

        if (WG(i) == m) //全是wire gate
            fit2 = (double)(1 - WG(i));

        else
            fit2 = (double)WG(i) / double(m);

        fit[i] = w1 * fit1 + w2 * fit2;
    }
}

void update()
{
    double lb_fit = 0.0, lw_fit = 100;
    int lb_index = 0, lw_index = 0;
    for (int i = 0; i < POPULATION; i++)
    {
        //update lb,lw
        if (fit[i] >= lb_fit)
        {
            lb_fit = fit[i];
            lb_index = i;
        }

        if (fit[i] <= lw_fit)
        {
            lw_fit = fit[i];
            lw_index = i;
        }
    }

    //update gb,gw
    if (lb_fit >= gb_fit)
    {
        gb_fit = lb_fit;
        for (int i = 0; i < n; i++) //update gb[]
        {
            for (int j = 0; j < m; j++)
            {
                gb[i][j] = X[lb_index][i][j];
            }
        }
    }

    if (lw_fit <= gw_fit)
    {
        gw_fit = lw_fit;
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < m; j++)
            {
                gw[i][j] = X[lw_index][i][j];
            }
        }
    }

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < m; j++)
        {
            if (gb[i][j] != gw[i][j]) //not same -> update
            {
                //update
                Q[i][j][gb[i][j]] += DELTA;
                Q[i][j][gw[i][j]] -= DELTA; 

                 //修正<0的case!!!
                if (Q[i][j][gw[i][j]] <= 0)
                {
                    Q[i][j][gw[i][j]] = 0;
                    Q[i][j][gb[i][j]] = 1;
                    for (int k = 0; k < STATE; k++)
                    {
                        if (k != gb[i][j])
                        {
                            Q[i][j][gb[i][j]] -= Q[i][j][k];
                        }
                    }
                }
            }
        }
    }
}

void print_toffoli()
{
    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < n; j++)
        {
            cout << gb[j][i] << " ";
        }
        cout << endl;
    }
}

void print_check_toffoli()
{
    cout << "=====for website======" << endl;
    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < n; j++)
        {
            if (gb[j][i] == 2)
                cout << 3 << " ";
            else if (gb[j][i] == 3)
                cout << 2 << " ";
            else
                cout << gb[j][i] << " ";
        }
        cout << endl;
    }
    cout << "===========" << endl;
}

int gate_cnt()
{
    int count = 0;
    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < n; j++)
        {
            if (gb[j][i] == 3)
            {
                count++;
                break;
            }
        }
    }
    return count;
}

int main()
{
    file << "Function\t找到答案的次數\t平均fitness\t平均gate數\t有找到答案的平均gate數\tm\t最佳解" << endl;
    for(int f = 0; f < 16; f++)
    {
        srand(114);
        double all_best = 0.0, sum_fit = 0.0, sum_gate = 0.0, sum_corret_gate = 0.0; //最佳fit/sum of fit/sum of gate/有正答案的sum of gate
        int get_ans_cnt = 0;
        int allbest_gate_cnt = 100;
        cout << "function:" << f << endl;
        n = n_arr[f];
        m = m_arr[f]; 
        file << "[";
        for(int i = 0; i < pow(2,n); i++)
        {
            output[i] = function[f][i];
            cout << output[i] << ",";
            file << output[i];
            if (i != pow(2, n) - 1)
            {
                file << ",";
            }
        }    
        file << "]\t";
        cout << endl;
        for (int i = 0; i < expTime; i++)
        {
            // cout << "exp: " << i << endl;
            init();
            for (int j = 0; j < ITERATION; j++)
            {
                measure();
                repair();
                fitness();
                update();
            }
            
            // if (gb_fit > all_best)
            // {
            //         all_best = gb_fit;
            // }
                
            if (gb_fit >= 1) //新的best有找到解
            {
                if(gate_cnt() <= allbest_gate_cnt)
                {
                    allbest_gate_cnt = gate_cnt();
                }
                get_ans_cnt++;
                sum_corret_gate += gate_cnt();
            }
            sum_fit += gb_fit;
            sum_gate += gate_cnt();
        }
        file << get_ans_cnt << "\t" << sum_fit / expTime << "\t" << sum_gate / expTime << "\t" << sum_corret_gate / get_ans_cnt << "\t" << m << "\t" << allbest_gate_cnt << endl;
        cout << get_ans_cnt << "\t" << sum_fit / expTime << "\t" << sum_gate / expTime << "\t" << sum_corret_gate / get_ans_cnt << "\t" << m << "\t" << allbest_gate_cnt << endl;
    }
    system("pause");
    return 0;
}
