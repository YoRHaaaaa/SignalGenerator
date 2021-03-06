#include "signalgenerator.h"
#include "ui_signalgenerator.h"
#include<QDateTime>
SignalGenerator::SignalGenerator(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SignalGenerator)
{
    ui->setupUi(this);

    //新建服务器设置对话框
    if(m_serversetting_dialog == nullptr)
    {
        m_serversetting_dialog = new ServerSetting(this);
        m_serversetting_dialog->setModal(true);
    }

    //放大器设置对话框
    if(m_ampsetting_dialog == nullptr)
    {
        m_ampsetting_dialog = new AmpSetting(this);
        m_ampsetting_dialog->setModal(true);
    }

    m_timer_gendata.setInterval(100);
    m_timer_gendata.setTimerType(Qt::PreciseTimer);
    connect(&m_timer_gendata, &QTimer::timeout, this, &SignalGenerator::on_m_timer_gendata_timeout);
    connect(m_serversetting_dialog, &ServerSetting::ServerStateChanged, this, [ = ]()
    {
        if(!m_serversetting_dialog->IsServerOn() && !m_timer_gendata.isActive())
        {
            m_timer_gendata.start();
        }
        else
        {
            m_timer_gendata.stop();
        }
    });


    m_timer_update_serverdata_s.setInterval(1000);
    m_timer_update_serverdata_s.setTimerType(Qt::PreciseTimer);
    connect(&m_timer_update_serverdata_s, &QTimer::timeout, this, [ = ]()
    {
        this->setWindowTitle("当前每秒发送帧数:" + QString::number(m_send_frame_s));
        m_send_frame_s = 0;
    });
    m_timer_update_serverdata_s.start();
}

SignalGenerator::~SignalGenerator()
{
    //删除服务器设置对话框
    if(m_serversetting_dialog != nullptr)
    {
        m_serversetting_dialog->deleteLater();
    }
    //删除放大器设置对话框
    if(m_ampsetting_dialog != nullptr)
    {
        m_ampsetting_dialog->deleteLater();
    }

    while(QLayoutItem *item = ui->m_verlayout_ch->takeAt(0))
    {
        if (QWidget *widget = item->widget())
        {
            widget->deleteLater();
        }
    }

    delete ui;
}

// 服务器设置点击
void SignalGenerator::on_m_setting_server_action_triggered()
{
    m_serversetting_dialog->show();
    m_serversetting_dialog->exec();
}


void SignalGenerator::on_m_setting_generator_action_triggered()
{
    m_ampsetting_dialog->show();
    if(m_ampsetting_dialog->exec())
    {
        ui->m_lineedit_srate->setText(m_ampsetting_dialog->GetSrate());
        m_timer_gendata.setInterval(1000 / m_ampsetting_dialog->GetSrate().toInt());
    }
}


void SignalGenerator::on_m_pushbutton_add_ch_clicked()
{
    ChannelWidget *channel_widget = new ChannelWidget(this);

    //添加导联默认名
    channel_widget->SetChannelName("ch" + QString::number(ui->m_lineedit_ch_num->text().toInt() + 1));
    AddChnnel(channel_widget);
}

void SignalGenerator::on_m_timer_gendata_timeout()
{
    static qint64 time_ms = 0;
    if(m_serversetting_dialog != nullptr && m_serversetting_dialog->IsServerOn())
    {
        QByteArray data;
        QByteArray tmp;
        tmp.resize(sizeof(float));
        for(int i = 0; i < ui->m_verlayout_ch->count(); i++)
        {
            ChannelWidget *chan_widget = qobject_cast<ChannelWidget *>(ui->m_verlayout_ch->itemAt(i)->widget());
            if(chan_widget != 0)
            {
                float ch_data = chan_widget->GenData(time_ms);
                memcpy(tmp.data(), &ch_data, sizeof(ch_data));
                data.push_back(tmp);
            }
        }

        m_serversetting_dialog->TcpSend(data);
        m_serversetting_dialog->SerialPortSend(data);

        time_ms++;
        m_send_frame_s++;

    }
}

void SignalGenerator::on_m_copy_channel_clicked(ChannelWidget *pChannelWidget)
{
    ChannelWidget *new_channelwidget = new ChannelWidget(this, pChannelWidget);
    AddChnnel(new_channelwidget);
}

void SignalGenerator::on_m_del_channel_clicked()
{
    ui->m_lineedit_ch_num->setText(QString::number(ui->m_lineedit_ch_num->text().toInt() - 1));
}

void SignalGenerator::UpDateChannelNum()
{
    int count = 0;
    for(int i = 0; i < ui->m_verlayout_ch->count(); i++)
    {
        ChannelWidget *chan_widget = qobject_cast<ChannelWidget *>(ui->m_verlayout_ch->itemAt(i)->widget());
        if(chan_widget != 0)
        {
            count++;
        }
    }

    ui->m_lineedit_ch_num->setText(QString::number(count));
}

void SignalGenerator::AddChnnel(ChannelWidget *new_ch_widget)
{
    ui->m_verlayout_ch->insertWidget(ui->m_verlayout_ch->indexOf(ui->m_pushbutton_add_ch), new_ch_widget);
    //复制导联
    connect(new_ch_widget, &ChannelWidget::CopyChannel, this, &SignalGenerator::on_m_copy_channel_clicked);

    //导联删除时更新主界面导联数
    connect(new_ch_widget, &ChannelWidget::ChannelDelete, this, &SignalGenerator::on_m_del_channel_clicked);
    UpDateChannelNum();
}

