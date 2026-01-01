// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>
#include <QMap>

class LocaleManager {
public:
	static LocaleManager &instance();

	void loadLocale(const QString &locale);
	QString translate(const QString &key, const QString &fallback = QString()) const;

	// Convenience method for formatted translations
	QString translate(const QString &key, const QStringList &args) const;

private:
	LocaleManager();
	~LocaleManager() = default;

	LocaleManager(const LocaleManager &) = delete;
	LocaleManager &operator=(const LocaleManager &) = delete;

	QMap<QString, QString> translations_;
};

// Convenience macros
#define Tr(key) LocaleManager::instance().translate(key)
#define TrArgs(key, ...) LocaleManager::instance().translate(key, QStringList{__VA_ARGS__})
