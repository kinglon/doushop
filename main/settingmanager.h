#pragma once

#include <QString>
#include <QVector>

class CSettingManager
{
protected:
	CSettingManager();

public:
	static CSettingManager* GetInstance();

private:
	void Load();

public:
    // 日志级别，默认info
    int m_nLogLevel = 2;

    // 请求间隔，单位号码
    int m_request_interval_ms = 100;

    // 是否启用webview日志输出
    bool m_enableWebviewLog = false;

    // 标志是否要缓存JS代码，false会实时从本地文件加载
    bool m_cacheJsCode = true;

    // 浏览器宽高
    int m_browserWidth = 1920;
    int m_browserHeight = 1080;
};
