#01_05
import bpy
import math
import bpy_extras
import gpu
import gpu_extras.batch
import copy
import mathutils
import json
import os

# ブレンダーに登録するアドオン情報
bl_info = {
    "name": "レベルエディタ",
    "author": "Yuto Furuya",
    "version": (1, 0),
    "blender": (3, 3, 1),
    "location": "",
    "description": "レベルエディタ",
    "warning": "",
    "support": "TESTING",
    "wiki_url": "",
    "tracker_url": "",
    "category": "Object"
}

# ゲームへ自動同期するJSONを作る
def make_live_sync_json():
    root = {"name": "scene", "objects": []}

    def parse_object(data_parent, obj):
        trans, rot, scale = obj.matrix_local.decompose()
        rot = rot.to_euler()
        item = {
            "type": obj.type,
            "name": obj.name,
            "transform": {
                "translation": [trans.x, trans.y, trans.z],
                "rotation": [
                    math.degrees(rot.x),
                    math.degrees(rot.y),
                    math.degrees(rot.z)
                ],
                "scaling": [scale.x, scale.y, scale.z]
            }
        }

        if "file_name" in obj:
            item["file_name"] = obj["file_name"]

        if "collider" in obj:
            item["collider"] = {
                "type": obj["collider"],
                "center": list(obj["collider_center"]),
                "size": list(obj["collider_size"])
            }

        data_parent.append(item)
        if obj.children:
            item["children"] = []
            for child in obj.children:
                parse_object(item["children"], child)

    for obj in bpy.context.scene.objects:
        if obj.parent is None:
            parse_object(root["objects"], obj)

    return root


def write_live_sync_json(scene):
    filepath = bpy.path.abspath(scene.myaddon_live_sync_path)
    if not filepath:
        return False

    directory = os.path.dirname(filepath)
    if directory:
        os.makedirs(directory, exist_ok=True)

    # C++側が書き込み途中のJSONを読まないよう、一時ファイルから置換する
    temporary_path = filepath + ".tmp"
    with open(temporary_path, "w", encoding="utf-8") as file:
        json.dump(make_live_sync_json(), file, ensure_ascii=False, indent=4)
    os.replace(temporary_path, filepath)
    return True


def export_live_sync_meshes(scene):
    if not scene.myaddon_live_sync_export_meshes:
        return

    json_path = bpy.path.abspath(scene.myaddon_live_sync_path)
    resources_directory = os.path.dirname(json_path)
    if not resources_directory:
        return
    os.makedirs(resources_directory, exist_ok=True)

    view_layer = bpy.context.view_layer
    old_active = view_layer.objects.active
    old_selected = list(bpy.context.selected_objects)

    try:
        for obj in scene.objects:
            file_name = str(obj.get("file_name", ""))
            if obj.type != 'MESH' or not file_name.lower().endswith(".obj"):
                continue

            for selected in list(bpy.context.selected_objects):
                selected.select_set(False)
            obj.select_set(True)
            view_layer.objects.active = obj

            # 座標・回転・拡大率はJSONから適用するため、OBJはローカル形状で出力する
            old_matrix = obj.matrix_world.copy()
            obj.matrix_world = mathutils.Matrix.Identity(4)
            output_path = os.path.join(resources_directory, file_name)
            try:
                if bpy.app.version >= (4, 0, 0):
                    bpy.ops.wm.obj_export(
                        filepath=output_path,
                        export_selected_objects=True,
                        export_materials=True,
                        export_triangulated_mesh=True,
                        forward_axis='NEGATIVE_Z',
                        up_axis='Y'
                    )
                else:
                    bpy.ops.export_scene.obj(
                        filepath=output_path,
                        use_selection=True,
                        use_materials=True,
                        use_triangles=True,
                        axis_forward='-Z',
                        axis_up='Y'
                    )
            finally:
                obj.matrix_world = old_matrix
    finally:
        for selected in list(bpy.context.selected_objects):
            selected.select_set(False)
        for selected in old_selected:
            if selected.name in view_layer.objects:
                selected.select_set(True)
        view_layer.objects.active = old_active


_live_sync_timer_pending = False
_live_sync_exporting = False


def live_sync_timer():
    global _live_sync_timer_pending, _live_sync_exporting
    _live_sync_timer_pending = False
    scene = bpy.context.scene
    if scene and scene.myaddon_live_sync_enabled:
        try:
            _live_sync_exporting = True
            export_live_sync_meshes(scene)
            write_live_sync_json(scene)
        except Exception as error:
            print("Live Sync export failed:", error)
        finally:
            _live_sync_exporting = False
    return None


def on_scene_changed(scene, depsgraph):
    global _live_sync_timer_pending
    if (_live_sync_exporting or not scene.myaddon_live_sync_enabled
            or _live_sync_timer_pending):
        return

    # 頂点移動中に毎回保存せず、最後の変更から少し遅らせて1回保存する
    _live_sync_timer_pending = True
    bpy.app.timers.register(live_sync_timer, first_interval=0.25)


class MYADDON_OT_live_sync_now(bpy.types.Operator):
    bl_idname = "myaddon.live_sync_now"
    bl_label = "Sync Now"
    bl_description = "現在のレベル配置をゲーム用JSONへ書き出します"

    def execute(self, context):
        if not context.scene.myaddon_live_sync_path:
            self.report({'ERROR'}, "先にtest.jsonの保存先を指定してください")
            return {'CANCELLED'}

        global _live_sync_exporting
        try:
            _live_sync_exporting = True
            export_live_sync_meshes(context.scene)
            write_live_sync_json(context.scene)
        finally:
            _live_sync_exporting = False
        self.report({'INFO'}, "ゲームへ同期しました")
        return {'FINISHED'}


class SCENE_PT_live_sync(bpy.types.Panel):
    bl_idname = "SCENE_PT_live_sync"
    bl_label = "Game Live Sync"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = 'scene'

    def draw(self, context):
        layout = self.layout
        layout.prop(context.scene, "myaddon_live_sync_path", text="test.json")
        layout.prop(context.scene, "myaddon_live_sync_enabled", text="Live Sync")
        layout.prop(
            context.scene,
            "myaddon_live_sync_export_meshes",
            text="Export OBJ Meshes"
        )
        layout.operator(MYADDON_OT_live_sync_now.bl_idname, icon='FILE_REFRESH')

#01_06
#オペレータ シーン出力
class MYADDON_OT_export_scene(bpy.types.Operator,bpy_extras.io_utils.ExportHelper):
    bl_idname = "myaddon.myaddon_ot_export_scene"
    bl_label = "シーン出力"
    bl_description = "シーン情報をExportします"
    #出力するファイルの拡張子
    filename_ext = ".json"

    def export_json(self):
        """JSON形式でファイルに出力"""

        #保存する情報をまとめるdict
        json_object_root = dict()

        #ノード名
        json_object_root["name"] = "scene"
        #オブジェクトリストを作成
        json_object_root["objects"] = list()

        #シーン内の全オブジェクトについて
        for object in bpy.context.scene.objects:
            
            #親オブジェクトがあるものはスキップ
            if(object.parent):
                continue

            #シーン直下のオブジェクトをルートノード(深さ0)とし、再起関数で走査
            self.parse_scene_recursive_json(json_object_root["objects"],object,0)

        #オブジェクトをJSON文字列にエンコード
        json_text = json.dumps(json_object_root,ensure_ascii=False, cls=json.JSONEncoder, indent=4)
        #コンソールに表示してみる
        print(json_text)

        #ファイルをテキスト形式で書きだし用にオープン
        #スコープを抜けると自動的にクローズされる
        with open(self.filepath, "wt", encoding="utf-8") as file:
            #ファイルに文字列を書き込む
            file.write(json_text)

    def parse_scene_recursive_json(self,data_parent, object, level):

        #シーンのオブジェクト１個分のjsonオブジェクト生成
        json_object = dict()
        #オブジェクト種類
        json_object["type"] = object.type
        #オブジェクト名
        json_object["name"] = object.name

        #オブジェクトのローカルトランスフォームから
        #平行移動、回転、スケールを抽出
        trans, rot, scale = object.matrix_local.decompose()
        #回転をQuarternionからEuler(3軸での回転角)に変換
        rot = rot.to_euler()
        #ラジアンから度数法に変換
        rot.x = math.degrees(rot.x)
        rot.y = math.degrees(rot.y)
        rot.z = math.degrees(rot.z)
        #トランスフォーム情報をディクショナリに登録
        transform = dict()
        transform["translation"] = (trans.x, trans.y, trans.z)
        transform["rotation"] = (rot.x, rot.y, rot.z)
        transform["scaling"] = (scale.x, scale.y, scale.z)
        #まとめて1個分のjsonオブジェクトに登録
        json_object["transform"] = transform
        #カスタムプロパティ'file_name'
        if "file_name" in object:
            json_object["file_name"] = object["file_name"]

        #カスタムプロパティ'collider'
        if"collider" in object:
            collider = dict()
            collider["type"] = object["collider"]
            collider["center"] = object["collider_center"].to_list()
            collider["size"] = object["collider_size"].to_list()
            json_object["collider"] = collider


        #１個分のjsonオブジェクトを親オブジェクトに登録
        data_parent.append(json_object)

        #子ノードがあれば
        if len(object.children) > 0:
            #子ノードリストを作成
            json_object["children"] = list()

            #子ノードへ進む(深さが1上がる)
            for child in object.children:
                self.parse_scene_recursive_json(json_object["children"],child,level + 1)


    def execute(self, context):
    
        print("シーン情報をExportします")
        
        #ファイルに出力
        self.export_json()
        

        print("シーン情報をExportしました")
        self.report({'INFO'}, "Scene Exported")

        return {'FINISHED'}
    
    def write_and_print(self, file, str):
        print(str)
        file.write(str)
        file.write("\n")
 
    def parse_scene_recursive(self, file, object, level):
        """シーン解析用再帰関数"""

        # 深さ分インデントする（タブを挿入）
        indent = ""
        for i in range(level):
            indent += "\t"

        # オブジェクト名書き込み
        self.write_and_print(file, indent + object.type)

        # ローカルトランスフォーム行列から平行移動、回転、スケーリングを抽出
        trans, rot, scale = object.matrix_local.decompose()

        # 回転を Quaternion から Euler（3軸での回転角）に変換
        rot = rot.to_euler()

        # ラジアンから度数法に変換
        rot.x = math.degrees(rot.x)
        rot.y = math.degrees(rot.y)
        rot.z = math.degrees(rot.z)

        # トランスフォーム情報を表示
        self.write_and_print(file, indent + "T(%f,%f,%f)" % (trans.x, trans.y, trans.z))
        self.write_and_print(file, indent + "R(%f,%f,%f)" % (rot.x, rot.y, rot.z))
        self.write_and_print(file, indent + "S(%f,%f,%f)" % (scale.x, scale.y, scale.z))
        
        #カスタムプロパティ'file_name'
        if "file_name" in object:
            self.write_and_print(file, indent + "N %s" % object["file_name"])
        #カスタムプロパティ'collision
        if"collider" in object:
            self.write_and_print(file, indent + "C %s" % object["collider"])
            temp_str = indent + "CC %f %f %f"
            temp_str %= (object["collider_center"][0],object["collider_center"][1],object["collider_center"][2])
            self.write_and_print(file,temp_str)
            temp_str = indent + "CS %f %f %f"
            temp_str %= (object["collider_size"][0],object["collider_size"][1],object["collider_size"][2])
            self.write_and_print(file,temp_str)

        self.write_and_print(file,indent + 'END')
        self.write_and_print(file, '')

        # 子ノードへ進む
        for child in object.children:
            self.parse_scene_recursive(file, child, level + 1)

    def export(self):
        """ファイルに出力"""
        print("シーン情報出力開始... %r" % self.filepath)
        #ファイルをテキスト形式で書き出し用にオープン
        #スコープを抜けると自動的にクローズされる
        with open(self.filepath, "wt") as file:
            #ファイルに文字列を書き込む
            self.write_and_print(file, "SCENE")

                        #シーン内の全オブジェクトについて
            for object in bpy.context.scene.objects:

                #親オブジェクトがあるものはスキップ
                if object.parent:
                    continue

                #シーン直下のオブジェクトをルートノードとして、再帰関数で走査
                self.parse_scene_recursive(file, object, 0)


#オペレータ ICO球生成
class MYADDON_OT_create_ico_sphere(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_create_object"
    bl_label = "ICO球生成"
    bl_description = "ICO球を生成します"
    bl_options = {'REGISTER', 'UNDO'}

    # メニューを実行したときに呼ばれる関数
    def execute(self, context):
        bpy.ops.mesh.primitive_ico_sphere_add()
        print("ICO球を生成しました。")

        return {'FINISHED'}

#オペレータ 頂点を伸ばす
class MYADDON_OT_stretch_vertex(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_stretch_vertex"
    bl_label = "頂点を伸ばす"
    bl_description = "頂点座標を引っ張って伸ばします"
    #リドゥ、アンドゥ可能オプション
    bl_options = {'REGISTER', 'UNDO'}

    #メニューを実行したときに呼ばれるコールバック関数
    def execute(self, context):
        bpy.data.objects["Cube"].data.vertices[0].co.x += 1.0
        print("頂点を伸ばしました。")

        #オペレータの命令終了を通知
        return {'FINISHED'}

#トップバーの拡張メニュー
class TOPBAR_MT_my_menu(bpy.types.Menu):
    #Blenderがクラスを識別する為の固有の文字列
    bl_idname = "TOPBAR_MT_my_menu" 	
    #メニューのラベルとして表示される文字列
    bl_label = "MyMenu" 		
    #著者表示用の文字列	
    bl_description = "拡張メニュー by " + bl_info["author"] 

    # サブメニューの描画
    def draw(self, context):

        #トップバーの「エディターメニュー」に項目（Manual）を追加
        self.layout.operator("wm.url_open_preset", 
            text="Manual", icon='HELP')

        #トップバーの「エディターメニュー」に項目（頂点を伸ばす）を追加
        self.layout.operator(MYADDON_OT_stretch_vertex.bl_idname,
        text=MYADDON_OT_stretch_vertex.bl_label)

        #トップバーの「エディターメニュー」に項目（ICO球生成）を追加
        self.layout.operator(MYADDON_OT_create_ico_sphere.bl_idname,
        text=MYADDON_OT_create_ico_sphere.bl_label)

        #トップバーの「エディターメニュー」に項目（Scene_export）を追加
        self.layout.operator(MYADDON_OT_export_scene.bl_idname,
        text=MYADDON_OT_export_scene.bl_label)


    # 既存のメニューにサブメニューを追加
    def submenu(self, context):

        # ID指定でサブメニューを追加
        self.layout.menu(TOPBAR_MT_my_menu.bl_idname)

#パネル ファイル名
class OBJECT_PT_file_name(bpy.types.Panel):
    bl_idname = "OBJECT_PT_file_name"
    bl_label = "FileName"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = 'object'

    #サブメニューの描画
    def draw(self, context):
        if "file_name" in context.object:
            #すでにプロパティがあれば、プロパティを表示
            self.layout.prop(context.object, '["file_name"]', text=self.bl_label)
        else:
            #プロパティがなければ、プロパティを追加ボタンを表示
            self.layout.operator(MYADDON_OT_add_filename.bl_idname)
        
        
        #self.layout.operator(MYADDON_OT_stretch_vertex.bl_idname,text=MYADDON_OT_stretch_vertex.bl_label)
        #self.layout.operator(MYADDON_OT_create_ico_sphere.bl_idname,text=MYADDON_OT_create_ico_sphere.bl_label)
        #self.layout.operator(MYADDON_OT_export_scene.bl_idname,text=MYADDON_OT_export_scene.bl_label)

        #self.layout.label(text="Hello")
        #self.layout.separator()
        #self.layout.label(text="Hello2",icon="MESH_CUBE")

#オペレータ　カスタムプロパティ['file_name']を追加
class MYADDON_OT_add_filename(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_add_filename"
    bl_label = "FileName 追加"
    bl_description = "['file_name']カスタムプロパティを追加します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):

        #['file_name']カスタムプロパティを追加
        context.object["file_name"] = ""

        return {'FINISHED'}
    
#オペレータ　カスタムプロパティ['collider']追加
class MYADDON_OT_add_collider(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_add_collider"
    bl_label = "コライダー 追加"
    bl_description = "['collider']カスタムプロパティを追加します"
    bl_options = {"REGISTER","UNDO"}

    def execute(self,context):

        #['collider']カスタムプロパティを追加
        context.object["collider"] = "BOX"
        context.object["collider_center"] = mathutils.Vector((0,0,0))
        context.object["collider_size"] = mathutils.Vector((2,2,2))

        return {"FINISHED"}

#パネル　コライダー
class OBJECT_PT_collider(bpy.types.Panel):
    bl_idname = "OBJECT_PT_collider"
    bl_label = "Collider"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"

    #サブメニューの描画
    def draw(self, context):

        #パネルに項目を追加
        if "collider" in context.object:
            #すでにプロパティがあれば、プロパティを表示
            self.layout.prop(context.object,'["collider"]',text="Type")
            self.layout.prop(context.object,'["collider_center"]',text="Center")
            self.layout.prop(context.object,'["collider_size"]',text="Size")
        else:
            #プロパティがなければ、プロパティ追加ボタン表示
            self.layout.operator(MYADDON_OT_add_collider.bl_idname)


#コライダー描画
class DrawCollider:
    
    # 描画ハンドル
    handle = None

    #3Dビューに登録する描画関数
    def draw_collider():

        #頂点データ
        vertices = {"pos":[]}
        #インデックスデータ
        indices = []
        
        #各頂点の、オブジェクト中心からのオフセット
        offsets = [
            [-0.5, -0.5, -0.5],#左下前
            [+0.5, -0.5, -0.5],#右下前
            [-0.5, +0.5, -0.5],#左上前
            [+0.5, +0.5, -0.5],#右上前
            [-0.5, -0.5, +0.5],#左下後
            [+0.5, -0.5, +0.5],#右下後
            [-0.5, +0.5, +0.5],#左上後
            [+0.5, +0.5, +0.5],#右上後
        ]
        #立方体のX,Y,Z方向のサイズ
        size = [2,2,2]

        #現在シーンのオブジェクトリストを走査
        for object in bpy.context.scene.objects:
            
            #コライダープロパティがなければ描画をスキップ
            if not "collider" in object:
                continue

            #中心点、サイズの変数を宣言
            center = mathutils.Vector((0,0,0))
            size = mathutils.Vector((2,2,2))

            #プロパティから値を取得
            center[0]=object["collider_center"][0]
            center[1]=object["collider_center"][1]
            center[2]=object["collider_center"][2]
            size[0]=object["collider_size"][0]
            size[1]=object["collider_size"][1]
            size[2]=object["collider_size"][2]

            #追加前の頂点数
            start = len(vertices["pos"])

            #Boxの8頂点数
            for offset in offsets:
                #オブジェクトの中心座標をコピー
                pos = copy.copy(center)
                #中心点を基準に各頂点ごとにずらす
                pos[0]+=offset[0]*size[0]
                pos[1]+=offset[1]*size[1]
                pos[2]+=offset[2]*size[2]
                #ローカル座標からワールド座標に変換
                pos = object.matrix_world @ pos
                #頂点データリストに座標を追加
                vertices['pos'].append(pos)
                #前面を構成する辺の頂点インデックス
                indices.append([start+0,start+1])
                indices.append([start+2,start+3])
                indices.append([start+0,start+2])
                indices.append([start+1,start+3])
                #奥面を構成する辺の頂点インデックス
                indices.append([start+4,start+5])
                indices.append([start+6,start+7])
                indices.append([start+4,start+6])
                indices.append([start+5,start+7])
                #手前遠くをつなぐ辺の頂点インデックス
                indices.append([start+0,start+4])
                indices.append([start+1,start+5])
                indices.append([start+2,start+6])
                indices.append([start+3,start+7])

        # ビルトインのシェーダを取得
        shader = gpu.shader.from_builtin('UNIFORM_COLOR')

        # バッチを作成(引数:シェーダ、トポロジー、頂点データ、インデックスデータ)
        batch = gpu_extras.batch.batch_for_shader(shader, 'LINES', vertices, indices=indices)

        # シェーダのパラメータ設定
        color = [0.5,1.0,1.0,1.0]
        shader.bind()
        shader.uniform_float("color", color)
        #描画
        batch.draw(shader)

       


# Blenderに登録するクラスリスト
classes = (
    MYADDON_OT_export_scene,
    MYADDON_OT_create_ico_sphere,
    MYADDON_OT_stretch_vertex,
    TOPBAR_MT_my_menu,
    OBJECT_PT_file_name,
    MYADDON_OT_add_filename,
    MYADDON_OT_add_collider,
    OBJECT_PT_collider
    ,MYADDON_OT_live_sync_now
    ,SCENE_PT_live_sync
)

        
#Add-On有効化時コールバック
def register():
    # Blenderにクラスを登録
    for cls in classes:
        bpy.utils.register_class(cls)
        
    #メニューに項目を追加
    bpy.types.TOPBAR_MT_editor_menus.append(TOPBAR_MT_my_menu.submenu)
    #3Dビューに描画関数を追加
    DrawCollider.handle = bpy.types.SpaceView3D.draw_handler_add(DrawCollider.draw_collider, (), 'WINDOW', 'POST_VIEW')

    bpy.types.Scene.myaddon_live_sync_path = bpy.props.StringProperty(
        name="Live Sync JSON",
        description="ゲーム側のresources/test.jsonを指定します",
        subtype='FILE_PATH',
        default=""
    )
    bpy.types.Scene.myaddon_live_sync_enabled = bpy.props.BoolProperty(
        name="Live Sync",
        description="Blenderの変更をゲームへ自動反映します",
        default=False
    )
    bpy.types.Scene.myaddon_live_sync_export_meshes = bpy.props.BoolProperty(
        name="Export OBJ Meshes",
        description="file_nameで指定したOBJも同じフォルダへ自動出力します",
        default=True
    )
    if on_scene_changed not in bpy.app.handlers.depsgraph_update_post:
        bpy.app.handlers.depsgraph_update_post.append(on_scene_changed)

    print("レベルエディタが有効化されました。")

#Add-On無効化時コールバック
def unregister():
    if on_scene_changed in bpy.app.handlers.depsgraph_update_post:
        bpy.app.handlers.depsgraph_update_post.remove(on_scene_changed)
    del bpy.types.Scene.myaddon_live_sync_export_meshes
    del bpy.types.Scene.myaddon_live_sync_enabled
    del bpy.types.Scene.myaddon_live_sync_path

    #メニューから項目を削除
    bpy.types.TOPBAR_MT_editor_menus.remove(TOPBAR_MT_my_menu.submenu)

    #3Dビューから描画関数を削除    
    bpy.types.SpaceView3D.draw_handler_remove(DrawCollider.handle, 'WINDOW')

    # Blenderからクラスを削除
    for cls in classes:
        bpy.utils.unregister_class(cls)
        print("レベルエディタが無効化されました。")


