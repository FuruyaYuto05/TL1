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
    #メニューに項目を追加
    bpy.types.TOPBAR_MT_editor_menus.append(draw_menu_manual)
    print("レベルエディタが有効化されました")
    
#アドオン無効化時コールバック
def unregister():
    #メニューから項目を削除
    bpy
    print("レベルエディタが無効化されました")
    
    
def draw_menu_manual(self, context):
    #self : 呼び出し元のクラスインスタンス C++で言うthisポインタ
    #context : カーソルを合わせたときのポップアップのカスタマイズなどに使用
    #トップバーの「エディターメニュー」に項目（オペレーター）を追加
    self.layout.operator("wm.url_open", text="Manual",icon='HELP')
    

if __name__ == "__main__":
   register()