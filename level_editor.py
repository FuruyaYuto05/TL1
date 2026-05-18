import bpy

bl_info = {
    "name": "レベルエディタ",
    "author": "Yuto Furuya",
    "version": (1, 0),
    "blender": (3, 3, 1),
    "location": "",
    "description": "レベルエディタ",
    "warning": "",
    "wiki_url": "",
    "tracker_url": "",
    "category": "Object",
}


#アドオン有効化時コールバック
def register():
    #クラスを登録
    for cls in classes:
        bpy.utils.register_class(cls)  

    #メニューに項目を追加
    bpy.types.TOPBAR_MT_editor_menus.append(TOPBAR_MT_my_menu.submenu)
    print("レベルエディタが有効化されました")
    
#アドオン無効化時コールバック
def unregister():
    #メニューから項目を削除
    bpy.types.TOPBAR_MT_editor_menus.remove(TOPBAR_MT_my_menu.submenu)
    
    #クラスを登録解除
    for cls in classes:
        bpy.utils.unregister_class(cls)
    
    print("レベルエディタが無効化されました")
    
    
def draw_menu_manual(self, context):
    #self : 呼び出し元のクラスインスタンス C++で言うthisポインタ
    #context : カーソルを合わせたときのポップアップのカスタマイズなどに使用
    #トップバーの「エディターメニュー」に項目（オペレーター）を追加
    self.layout.operator("wm.url_open", text="Manual",icon='HELP')
    
class TOPBAR_MT_my_menu(bpy.types.Menu):
    bl_idname = "TOPBAR_MT_my_menu"
    bl_label = "My Menu"
    bl_description = "拡張メニュー by" + bl_info["author"]

    #サブメニュー描画
    def draw(self, context):
       #トップバーの「エディターメニュー」に項目（オペレーター）を追加
       self.layout.operator("wm.url_open_preset", text="Manual", icon='HELP')

    #既存のメニューにサブメニュー追加
    def submenu(self, context):
       #ID名を指定して、サブメニューを追加
       self.layout.menu(TOPBAR_MT_my_menu.bl_idname)

#Blemderに登録するクラスリスト
classes = (
    TOPBAR_MT_my_menu,
)

if __name__ == "__main__":
   register()