#ifndef UI_COMMAND_PALETTE_H
#define UI_COMMAND_PALETTE_H

/*
File ui_command_palette.h — Open-Timetable addition

This file is part of Open-Timetable, a modernised user interface for the
timetabling engine. Licensed under the GNU Affero General Public License v3.

«كل الأدوات»: نافذة بحث واحدة تعرض **كل أوامر البرنامج** (كل إجراءات النافذة
الرئيسية، بما فيها تلك التي كانت أزرارًا في الواجهة القديمة)، مع اسم القائمة
التي ينتمي إليها كل أمر. تُفتح بـ Ctrl+Shift+P أو من الشريط الجانبي.

بهذا لا تُفقد أي ميزة من البرنامج الأصلي: كل أمر له وجود، ويُوجد بالبحث.
*/

class QMainWindow;

namespace UiCommandPalette {
	// يفتح نافذة البحث عن الأوامر (نافذة واحدة فقط في كل مرة).
	void run(QMainWindow* window);
}

#endif // UI_COMMAND_PALETTE_H
