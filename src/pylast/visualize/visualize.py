"""
Written by:
    @author: Yiyun Huang
    @email: 
    @date: 2025-04-28
"""
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
from matplotlib.patches import Circle, Rectangle, Ellipse
from mpl_toolkits.axes_grid1 import make_axes_locatable
from typing import Dict, List, Optional, Tuple, Union
from dataclasses import dataclass
import sys

# ==================== 数据模型定义 ====================
@dataclass
class TelescopeGeometry:
    """望远镜几何信息"""
    tel_id: int
    pos_x: float  # X坐标 (m)
    pos_y: float  # Y坐标 (m)
    focal_length: float  # 焦距 (cm)
    pix_x: np.ndarray  # 像素X坐标 (cm)
    pix_y: np.ndarray  # 像素Y坐标 (cm)
    pix_size: np.ndarray  # 像素尺寸 (cm)

@dataclass
class EventData:
    """事件数据容器"""
    event_id: int
    mce0: float  # 能量 (TeV)
    mcxcore: float  # 核心X (m)
    mcycore: float  # 核心Y (m)
    mcze: float  # 天顶角 (deg)
    mcaz: float  # 方位角 (deg)
    xmax: float  # Xmax (g/cm^2)
    hini: float  # 首次相互作用高度 (m)
    pe_data: Dict[int, np.ndarray]  # 各望远镜的光电子数 (tel_id: pe_counts)
    sum_pe: np.ndarray  # 各望远镜总光电子数
    triggered_tels: np.ndarray  # 触发望远镜ID数组

@dataclass
class HillasParameters:
    """Hillas参数容器"""
    length: float  # 长度 (cm)
    width: float  # 宽度 (cm)
    psi: float  # 方向角 (deg)
    cog_x: float  # 重心X (cm)
    cog_y: float  # 重心Y (cm)

def read_event_data(event, tel_geoms: Dict[int, TelescopeGeometry]) -> EventData:
    """从事件对象提取结构化数据"""
    pe_data = {}
    sum_pe = []
    for tel_id in tel_geoms:
        pe_data[tel_id] = event.simulation.tels[tel_id].true_image
        sum_pe.append(event.simulation.tels[tel_id].true_image_sum)
    
    triggered = np.array([tel_id for tel_id, pe in pe_data.items() if np.sum(pe) > 0])
    
    return EventData(
        event_id=getattr(event, 'count', 0),
        mce0=event.simulation.shower.energy,
        mcxcore=event.simulation.shower.core_x,
        mcycore=event.simulation.shower.core_y,
        mcze=event.simulation.shower.alt * 180 / np.pi,
        mcaz=event.simulation.shower.az * 180 / np.pi,
        xmax=event.simulation.shower.x_max,
        hini=event.simulation.shower.h_first_int,
        pe_data=pe_data,
        sum_pe=np.array(sum_pe),
        triggered_tels=triggered
    )

def event_summary(events: List, tel_geoms: Dict[int, TelescopeGeometry]) -> Dict[str, Union[float, int]]:
    """
    计算并打印事件摘要统计信息
    
    参数:
        events: 事件对象列表
        tel_geoms: 望远镜几何信息字典
        
    返回:
        包含统计信息的字典:
        {
            'total_events': 总事件数,
            'min_energy': 最小能量,
            'max_energy': 最大能量,
            'min_npe': 最小总光电子数,
            'max_npe': 最大总光电子数,
            'min_energy_event': 最小能量事件索引,
            'max_energy_event': 最大能量事件索引,
            'min_npe_event': 最小光电子数事件索引,
            'max_npe_event': 最大光电子数事件索引
        }
    """
    stats = {
        'total_events': len(events),
        'min_energy': float('inf'),
        'max_energy': -float('inf'),
        'min_npe': float('inf'),
        'max_npe': -float('inf'),
        'min_energy_event': -1,
        'max_energy_event': -1,
        'min_npe_event': -1,
        'max_npe_event': -1
    }
    
    triggered_count = 0
    
    for i, event in enumerate(events):
        event_data = read_event_data(event, tel_geoms)
        
        if len(event_data.triggered_tels) == 0:
            continue
            
        triggered_count += 1
            
        # 能量统计
        if event_data.mce0 < stats['min_energy']:
            stats['min_energy'] = event_data.mce0
            stats['min_energy_event'] = i
        if event_data.mce0 > stats['max_energy']:
            stats['max_energy'] = event_data.mce0
            stats['max_energy_event'] = i
            
        # 光电子数统计
        total_npe = np.sum(event_data.sum_pe)
        if total_npe < stats['min_npe']:
            stats['min_npe'] = total_npe
            stats['min_npe_event'] = i
        if total_npe > stats['max_npe']:
            stats['max_npe'] = total_npe
            stats['max_npe_event'] = i
    
    # 自动打印统计信息
    print("\n=== 事件统计摘要 ===")
    print(f"总事件数: {stats['total_events']}")
    print(f"触发事件数: {triggered_count}")
    print(f"\n能量统计:")
    print(f"  最小值: {stats['min_energy']:.2f} TeV (事件 {stats['min_energy_event']})")
    print(f"  最大值: {stats['max_energy']:.2f} TeV (事件 {stats['max_energy_event']})")
    print(f"\n光电子数统计:")
    print(f"  最小值: {stats['min_npe']:.2f} (事件 {stats['min_npe_event']})")
    print(f"  最大值: {stats['max_npe']:.2f} (事件 {stats['max_npe_event']})")
    print("==================\n")
    
    return stats

def find_closest_event(events: List, tel_geoms: Dict[int, TelescopeGeometry],
                      target_energy: Optional[float] = None,
                      target_npe: Optional[float] = None,
                      target_ntel: Optional[int] = None,
                      target_xmax: Optional[float] = None) -> Optional[int]:
    """
    查找最接近目标参数的事件
    
    参数:
        events: 事件对象列表
        tel_geoms: 望远镜几何信息字典
        target_energy: 目标能量 (TeV)
        target_npe: 目标总光电子数
        target_ntel: 目标触发望远镜数量
        target_xmax: 目标Xmax值 (g/cm^2)
        
    返回:
        最接近的事件索引，如果没有满足条件的事件则返回None
    """
    if all(v is None for v in [target_energy, target_npe, target_ntel, target_xmax]):
        return None
        
    best_event = None
    min_distance = float('inf')
    
    for i, event in enumerate(events):
        event_data = read_event_data(event, tel_geoms)
        
        if len(event_data.triggered_tels) == 0:
            continue
            
        distance = 0
        weights = 0
        
        if target_energy is not None:
            distance += ((event_data.mce0 - target_energy) / target_energy) ** 2
            weights += 1
        if target_npe is not None:
            total_npe = np.sum(event_data.sum_pe)
            distance += ((total_npe - target_npe) / target_npe) ** 2
            weights += 1
        if target_ntel is not None:
            distance += ((len(event_data.triggered_tels) - target_ntel) / target_ntel) ** 2
            weights += 1
        if target_xmax is not None:
            distance += ((event_data.xmax - target_xmax) / target_xmax) ** 2
            weights += 1
            
        if weights > 0:
            distance = np.sqrt(distance / weights)
            
            if distance < min_distance:
                min_distance = distance
                best_event = i
                
    return best_event

# ==================== 可视化器实现 ====================
class Visualizer:
    """数据可视化器（直接使用源数据初始化）"""
    def __init__(self, source_data):
        """
        初始化可视化器
        Args:
            source_data: 直接传入的原始数据源
        """
        self.source_data = source_data
        self._load_telescope_data()
    
    def _load_telescope_data(self):
        """加载望远镜几何数据"""
        self.tel_geoms = {}
        for tel_id, tel_coord in self.source_data.subarray.tel_positions.items():
            self.tel_geoms[tel_id] = TelescopeGeometry(
                tel_id=tel_id,
                pos_x=tel_coord[0],
                pos_y=tel_coord[1],
                focal_length=self.source_data.subarray.tels[tel_id].optics.equivalent_focal_length * 100,
                pix_x=self.source_data.subarray.tels[tel_id].camera.geometry.pix_x * 100,
                pix_y=self.source_data.subarray.tels[tel_id].camera.geometry.pix_y * 100,
                pix_size=np.sqrt(self.source_data.subarray.tels[tel_id].camera.geometry.pix_area) * 100
            )
    
    def visualize_telpos(self, event, output_path: Optional[str] = None):
        """
        可视化望远镜位置和触发情况
        Args:
            event: 直接传入的事件数据对象
            output_path: 输出文件路径(可选)
        """
        event_data = read_event_data(event, self.tel_geoms)
        
        # 准备数据
        tel_positions = np.array([(geom.pos_x, geom.pos_y) for geom in self.tel_geoms.values()])
        x = tel_positions[:, 0]
        y = tel_positions[:, 1]
        log_npe = np.log10(np.clip(event_data.sum_pe, 1, None))
        triggered = np.isin([tel.tel_id for tel in self.tel_geoms.values()], event_data.triggered_tels)
        
        # 创建图形
        fig, ax = plt.subplots(figsize=(10, 8))
        
        # 绘制望远镜
        scatter = ax.scatter(x, y, c=log_npe, cmap='viridis', s=300, edgecolor='k', alpha=0.8)
        ax.scatter(x[triggered], y[triggered], facecolors='none', edgecolors='r', s=350, linewidths=2)
        
        # 标记望远镜ID
        for tel in self.tel_geoms.values():
            ax.text(tel.pos_x, tel.pos_y+30, str(tel.tel_id),
                   color='black', fontsize=12, ha='center', va='center')
        
        # 绘制簇射核心和方向
        ax.scatter(event_data.mcxcore, event_data.mcycore, color='red', marker='*', s=300, label='Shower Core')
        direction_x = -np.sin(np.radians(event_data.mcaz))
        direction_y = np.cos(np.radians(event_data.mcaz))
        ax.plot([event_data.mcxcore, event_data.mcxcore + direction_x * 800],
                [event_data.mcycore, event_data.mcycore + direction_y * 800],
                color='red', linestyle='--', label='Shower Direction')
        
        # 设置图形属性
        ax.set_xlabel("X Position (m)")
        ax.set_ylabel("Y Position (m)")
        ax.set_title("Telescope Positions and Shower Core")
        ax.legend()
        cbar = fig.colorbar(scatter, ax=ax, label="log(NPE)")
        plt.xlim(-850, 850)
        plt.ylim(-850, 850)
        
        if output_path:
            plt.savefig(output_path, dpi=600)
        plt.show()
    
    def visualize_event(self, event, output_path: Optional[str] = None):
        """
        可视化事件图像
        Args:
            event: 直接传入的事件数据对象
            output_path: 输出文件路径(可选)
        """
        event_data = read_event_data(event, self.tel_geoms)
        hillas_params = self._get_hillas_parameters(event)
        
        num_triggered = len(event_data.triggered_tels)
        if num_triggered == 0:
            print("No triggered telescopes for this event")
            return
            
        num_cols = int(np.sqrt(num_triggered + 1))
        num_rows = (num_triggered + num_cols) // num_cols
        
        fig, axes = plt.subplots(num_rows, num_cols, figsize=(6 * num_cols + 2, 6 * num_rows))
        axes = axes.flatten() if num_triggered > 1 else [axes]
        
        colors = ['white'] + plt.cm.plasma(np.linspace(0, 1, 256)).tolist()
        cmap = mcolors.ListedColormap(colors)
        
        for i, tel_id in enumerate(event_data.triggered_tels):
            ax = axes[i+1]
            tel_geom = self.tel_geoms[tel_id]
            pe_data = event_data.pe_data[tel_id]
            
            max_pe = np.max(pe_data)
            bounds = [0, 1] + np.linspace(1, max_pe, 256).tolist()
            norm = mcolors.BoundaryNorm(bounds, cmap.N)
            
            self._draw_camera_image(ax, tel_geom, pe_data, norm, cmap)
            
            if tel_id in hillas_params:
                self._draw_hillas_ellipse(ax, hillas_params[tel_id])
            
        # 主事件信息
        axes[0].axis('off')
        info_text = (
            #f"Event {event_data.event_id}\n"
            f"Energy: {event_data.mce0:.2f} TeV\n"
            f"Core: ({event_data.mcxcore:.1f}, {event_data.mcycore:.1f}) m\n"
            f"Direction: (ze={event_data.mcze:.1f}°, az={event_data.mcaz:.1f}°)\n"
            f"Xmax: {event_data.xmax:.1f} g/cm²\n"
            f"First Int: {event_data.hini:.1f} m\n"
            f"Triggered Tels: {num_triggered}"
        )
        axes[0].text(0.5, 0.5, info_text, fontsize=25,fontweight='bold',
                    ha='center', va='center', transform=axes[0].transAxes)
        
        # 关闭多余子图
        for ax in axes[num_triggered+1:]:
            ax.axis('off')
            
        plt.tight_layout(pad=1.0)
        if output_path:
            plt.savefig(output_path, dpi=600)
        plt.show()
    
    def _get_hillas_parameters(self, event) -> Dict[int, HillasParameters]:
        """从事件对象提取Hillas参数"""
        # 添加对dl1属性是否存在的检查
        if not hasattr(event, 'dl1') or event.dl1 is None or not hasattr(event.dl1, 'tels'):
            return {}
        
        # 添加对tels是否为空的检查
        if not event.dl1.tels:
            return {}
            
        hillas_params = {}
        for tel_id, dl1 in event.dl1.tels.items():
            # 确保望远镜存在于几何信息中
            if tel_id not in self.tel_geoms:
                continue
                
            tel_geom = self.tel_geoms[tel_id]
            
            # 检查hillas参数是否存在
            if not hasattr(dl1, 'image_parameters') or not hasattr(dl1.image_parameters, 'hillas'):
                continue
            tel_geom = self.tel_geoms[tel_id]
            params = dl1.image_parameters.hillas
            hillas_params[tel_id] = HillasParameters(
                length=params.length * tel_geom.focal_length * 2,
                width=params.width * tel_geom.focal_length * 2,
                psi=params.psi * 180 / np.pi,
                cog_x=params.x * tel_geom.focal_length,
                cog_y=params.y * tel_geom.focal_length
            )
        return hillas_params
    
    def _draw_camera_image(self, ax, tel_geom: TelescopeGeometry,
                         pe_data: np.ndarray, norm, cmap):
        """绘制相机图像"""
        for x, y, size, pe in zip(tel_geom.pix_x, tel_geom.pix_y, tel_geom.pix_size, pe_data):
            color = cmap(norm(pe))
            rect = Rectangle((x-size/2, y-size/2), size, size,
                           facecolor=color, edgecolor='k')
            ax.add_patch(rect)
            
        ax.set_xlim(tel_geom.pix_x.min()-tel_geom.pix_size.max(),
                   tel_geom.pix_x.max()+tel_geom.pix_size.max())
        ax.set_ylim(tel_geom.pix_y.min()-tel_geom.pix_size.max(),
                   tel_geom.pix_y.max()+tel_geom.pix_size.max())
        ax.set_aspect('equal')
        ax.set_xlabel('X Position (cm)')
        ax.set_ylabel('Y Position (cm)')
        
        # 添加角度坐标轴
        secax_x = ax.secondary_xaxis('top',
            functions=(lambda x: np.degrees(np.arctan(x / tel_geom.focal_length)),
                      lambda x: np.tan(np.radians(x)) * tel_geom.focal_length))
        secax_x.set_xlabel('X (degrees)')
        
        secax_y = ax.secondary_yaxis('right',
            functions=(lambda y: np.degrees(np.arctan(y / tel_geom.focal_length)),
                      lambda y: np.tan(np.radians(y)) * tel_geom.focal_length))
        secax_y.set_ylabel('Y (degrees)')
        
        # 添加颜色条
        divider = make_axes_locatable(ax)
        cax = divider.append_axes("right", size="5%", pad=0.6)
        sm = plt.cm.ScalarMappable(cmap=cmap, norm=norm)
        sm.set_array([])
        plt.colorbar(sm, cax=cax, label='PE')
    
    def _draw_hillas_ellipse(self, ax, hillas: HillasParameters):
        """绘制Hillas椭圆"""
        ellipse = Ellipse(
            xy=(hillas.cog_x, hillas.cog_y),
            width=hillas.length,
            height=hillas.width,
            angle=hillas.psi,
            edgecolor='r', facecolor='none', lw=2
        )
        ax.add_patch(ellipse)
        ax.plot(hillas.cog_x, hillas.cog_y, 'rx')
