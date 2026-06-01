import bpy
import math

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

       self.layout.operator(MYADDON_OT_stretch_vertex.bl_idname, text=MYADDON_OT_stretch_vertex.bl_label)

       self.layout.operator(MYADDON_OT_create_ico_sphere.bl_idname, text=MYADDON_OT_create_ico_sphere.bl_label)

       self.layout.operator(MYADDON_OT_export_scece.bl_idname, text=MYADDON_OT_export_scece.bl_label)

    #既存のメニューにサブメニュー追加
    def submenu(self, context):
       #ID名を指定して、サブメニューを追加
       self.layout.menu(TOPBAR_MT_my_menu.bl_idname)

class MYADDON_OT_stretch_vertex(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_stretch_vertex"
    bl_label = "頂点を伸ばす"
    bl_description = "頂点座標を引っ張って伸ばします"
    #リドゥ、アンドゥ可能オプション
    bl_options = {'REGISTER', 'UNDO'}

    #メニューを実行したときに呼ばれるコールバック関数
    def execute(self, context):
        bpy.data.objects["Cube"].data.vertices[0].co.x += 1.0
        print("頂点を伸ばしました")
        
        #オペレーターが頂点命令終了を通知
        return {'FINISHED'} 

class MYADDON_OT_create_ico_sphere(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_create_object"
    bl_label = "ico sphereを作成"
    bl_description = "ico sphereを作成します"
    #リドゥ、アンドゥ可能オプション
    bl_options = {'REGISTER', 'UNDO'}

    #メニューを実行したときに呼ばれるコールバック関数
    def execute(self, context):
        bpy.ops.mesh.primitive_ico_sphere_add()
        print("ico sphereを作成しました")
        
        #オペレーターが頂点命令終了を通知
        return {'FINISHED'} 

class MYADDON_OT_export_scece(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_export_scene"
    bl_label = "シーン出力"
    bl_description = "シーン情報をエクスポートします"
    

    #メニューを実行したときに呼ばれるコールバック関数
    def execute(self, context):
        print("シーン情報をエクスポートします")
        
        for object in bpy.context.scene.objects:
            print(object.type + " -" + object.name)
            #ローカルトランスフォーム行列から平行移動、回転、スケーリングを抽出
            #型は Vector, Quaternion, Vector
            trans, rot, scale = object.matrix_local.decompose()
            #回転を Quaternion から Euler (3軸での回転角)に変換
            rot = rot.to_euler()
            #ラジアンから度数法に変換
            rot.x = math.degrees(rot.x)
            rot.y = math.degrees(rot.y)
            rot.z = math.degrees(rot.z)

            #トランスフォーム情報を表示
            print("Trans(%f,%f,%f)" % (trans.x, trans.y, trans.z))
            print("Rot(%f,%f,%f)" % (rot.x, rot.y, rot.z))
            print("Scale(%f,%f,%f)" % (scale.x, scale.y, scale.z))
            
            #親オブジェクトの名前を表示
            if object.parent:
                print("Parent: " + object.parent.name)
            print()

        print("シーン情報をエクスポートしました")
        self.report({'INFO'}, "シーン情報をエクスポートしました")

        #オペレーターが頂点命令終了を通知
        return {'FINISHED'}


#Blemderに登録するクラスリスト
classes = (
    MYADDON_OT_stretch_vertex,
    MYADDON_OT_create_ico_sphere,
    TOPBAR_MT_my_menu,
    MYADDON_OT_export_scece,
)

if __name__ == "__main__":
   register()