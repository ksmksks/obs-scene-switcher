// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "locale_manager.hpp"

#include <obs-module.h>
#include <QFile>
#include <QTextStream>

LocaleManager &LocaleManager::instance()
{
	static LocaleManager inst;
	return inst;
}

LocaleManager::LocaleManager()
{
	// OBS のロケール設定を取得
	const char *locale = obs_get_locale();
	if (locale) {
		loadLocale(QString::fromUtf8(locale));
	} else {
		loadLocale("en-US");
	}
}

void LocaleManager::loadLocale(const QString &locale)
{
	translations_.clear();

	// ロケールファイルのパスを構築
	QString localeFileName = QString("locale/%1.ini").arg(locale);
	char *path = obs_module_file(localeFileName.toUtf8().constData());

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		// フォールバック: en-US
		blog(LOG_DEBUG, "[obs-scene-switcher] Locale file not found: %s, trying en-US", path);
		bfree(path);
		path = obs_module_file("locale/en-US.ini");
		file.setFileName(path);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			blog(LOG_WARNING, "[obs-scene-switcher] Failed to load locale file");
			bfree(path);
			return;
		}
	}

	bfree(path);

	QTextStream in(&file);
	in.setEncoding(QStringConverter::Utf8);

	while (!in.atEnd()) {
		QString line = in.readLine().trimmed();

		// コメントまたは空行をスキップ
		if (line.isEmpty() || line.startsWith('#'))
			continue;

		// key="value" 形式をパース
		int equalPos = line.indexOf('=');
		if (equalPos > 0) {
			QString key = line.left(equalPos).trimmed();
			QString value = line.mid(equalPos + 1).trimmed();

			// 前後の " を削除
			if (value.startsWith('"') && value.endsWith('"')) {
				value = value.mid(1, value.length() - 2);
			}

			translations_[key] = value;
		}
	}

	blog(LOG_DEBUG, "[obs-scene-switcher] Loaded %d translations for locale %s", translations_.size(),
	     locale.toUtf8().constData());
}

QString LocaleManager::translate(const QString &key, const QString &fallback) const
{
	if (translations_.contains(key)) {
		return translations_[key];
	}

	return fallback.isEmpty() ? key : fallback;
}

QString LocaleManager::translate(const QString &key, const QStringList &args) const
{
	QString translated = translate(key);

	// %1, %2, ... を引数で置換
	for (int i = 0; i < args.size(); ++i) {
		translated = translated.arg(args[i]);
	}

	return translated;
}
