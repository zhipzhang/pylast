import numpy as np
from scipy.stats import binned_statistic_2d
from scipy.interpolate import RegularGridInterpolator
from astropy.convolution import Gaussian2DKernel, convolve
from typing import Optional
import abc

def root_mean_square(values):
    """
    计算一个数组的均方根 (RMS)。
    
    参数:
    values (np.ndarray): 输入的数组，代表一个 bin 内的所有值。
    
    返回:
    float: 数组的均方根。如果数组为空，则返回 NaN。
    """
    # 如果 bin 是空的，numpy 会自动处理并返回 NaN，这是正确的行为。
    # 无需特殊处理空数组。
    return np.sqrt(np.mean(np.square(values)))

class Lookup2DTable(abc.ABC):
    def __init__(self, x_bins:np.ndarray, y_bins:np.ndarray):
        if x_bins.ndim != 1 or y_bins.ndim != 1:
            raise ValueError("x_bins and y_bins must be 1D arrays")
        self.x_bins = x_bins
        self.y_bins = y_bins
        self.x_centers = 0.5 * (x_bins[1:] + x_bins[:-1])
        self.y_centers = 0.5 * (y_bins[1:] + y_bins[:-1])
        self.counts_map = None
        self.data_map = None
        self.smoothed_data_map = None
        self._interpolator:Optional[RegularGridInterpolator] = None


    def fill_counts_map(self, x:np.ndarray, y:np.ndarray):
        self.counts_map, _, _, _ = binned_statistic_2d(x, y, None, bins=(self.x_bins, self.y_bins), statistic='count')

    def smooth(self, sigma = 2):
        if self.data_map is None:
            raise ValueError("data_map is not filled")
        kernel = Gaussian2DKernel(x_stddev=sigma)
        self.smoothed_data_map = convolve(self.data_map, kernel, boundary='extend', nan_treatment='interpolate')
        interpolating_map = self.smoothed_data_map.copy()
        interpolating_map[np.isnan(interpolating_map)] = 0

        self._interpolator = RegularGridInterpolator((self.x_centers, self.y_centers), interpolating_map, bounds_error=False, fill_value=None)
    def display(self, ax=None, original_data = False, log_scale=False, show_counts=False, **kwargs):
        """Display the smoothed data map as a 2D plot.
        
        Parameters
        ----------
        ax : matplotlib.axes.Axes, optional
            Axes to plot on. If None, creates new figure and axes.
        log_scale : bool, optional
            If True, use logarithmic scale for the colorbar. Default is False.
        original_data : bool, optional
            If True, display the original data map. Default is False.
        show_counts : bool, optional
            If True, display the counts map. Default is False.
        **kwargs
            Additional keyword arguments passed to imshow.
            
        Returns
        -------
        matplotlib.image.AxesImage
            The image object returned by imshow.
        """
        if show_counts:
            if self.counts_map is None:
                raise ValueError("counts_map is not filled. Call fill_counts_map() first.")
        elif self.smoothed_data_map is None:
            raise ValueError("smoothed_data_map is not filled. Call smooth() first.")
        
        import matplotlib.pyplot as plt
        from matplotlib.colors import LogNorm
        
        if ax is None:
            fig, ax = plt.subplots(figsize=(8, 6))
        
        # Set default kwargs for imshow
        imshow_kwargs = {
            'extent': [self.x_bins[0], self.x_bins[-1], self.y_bins[0], self.y_bins[-1]],
            'origin': 'lower',
            'aspect': 'auto',
            'cmap': 'viridis'
        }
        
        # Add LogNorm if log_scale is True
        if log_scale:
            imshow_kwargs['norm'] = LogNorm()
        
        imshow_kwargs.update(kwargs)
        
        if show_counts:
            im = ax.imshow(self.counts_map.T, **imshow_kwargs)
            ax.set_title('Counts Map')
        elif original_data: 
            im = ax.imshow(self.data_map.T, **imshow_kwargs)
            ax.set_title('Data Map')
        else:
            im = ax.imshow(self.smoothed_data_map.T, **imshow_kwargs)
            ax.set_title('Data Map')
        ax.set_xlabel('X')
        ax.set_ylabel('Y')
        
        # Add colorbar
        plt.colorbar(im, ax=ax)
        
        return im
    
    def save(self, filename:str):
        if self.smoothed_data_map is None:
            raise ValueError("smoothed_data_map is not filled")
        np.savez(filename, x_bins=self.x_bins, y_bins=self.y_bins, data_map=self.data_map, counts_map=self.counts_map, x_centers=self.x_centers, y_centers=self.y_centers, smoothed_data_map=self.smoothed_data_map)
    @classmethod
    def load(cls, filename:str):
        instance = cls.__new__(cls)
        with np.load(filename) as data:
            instance.x_bins = data['x_bins']
            instance.y_bins = data['y_bins']
            instance.data_map = data['data_map']
            instance.counts_map = data['counts_map']
            instance.x_centers = data['x_centers']
            instance.y_centers = data['y_centers']
            instance.smoothed_data_map = data['smoothed_data_map']
            instance._interpolator = RegularGridInterpolator((instance.x_centers, instance.y_centers), instance.smoothed_data_map, bounds_error=False, fill_value=None)
        return instance
    @abc.abstractmethod
    def fill(self, x, y, z):
        pass
    def __call__(self, x, y):
        return self._interpolator((x, y))


class SigmaLookupTable(Lookup2DTable):
    def __init__(self, x_bins:np.ndarray, y_bins:np.ndarray, min_counts = 100):
        super().__init__(x_bins, y_bins)
        self.min_counts = min_counts
    def fill(self, x, y, z):
        self.fill_counts_map(x, y)
        self.data_map, _, _, _= binned_statistic_2d(x, y, z, bins=(self.x_bins, self.y_bins), statistic=root_mean_square)
        self.data_map[self.counts_map < self.min_counts] = np.nan
    def __call__(self, x, y):
        return super().__call__(x, y)


class SigmaLookupTableCollection:
    """
    管理多个 SigmaLookupTable 对象的集合类
    基于 offset_bins 区间来管理不同的表格
    类似于三维查找表：(x, y, offset) -> value
    """
    
    def __init__(self, x_bins: np.ndarray, y_bins: np.ndarray, 
                 offset_bins: np.ndarray, min_counts: int = 100):
        """
        初始化集合
        
        Parameters
        ----------
        x_bins : np.ndarray
            X轴bin边界，如 [0, 1, 2, ..., 10]
        y_bins : np.ndarray  
            Y轴bin边界，如 [0, 1, 2, ..., 10]
        offset_bins : np.ndarray
            Offset bin边界，如 [0, 1, 2, 3, 4, 5]
            定义了5个区间：[0,1), [1,2), [2,3), [3,4), [4,5]
        min_counts : int
            最小计数阈值
        """
        if offset_bins.ndim != 1:
            raise ValueError("offset_bins must be 1D array")
        
        self.x_bins = x_bins
        self.y_bins = y_bins
        self.offset_bins = offset_bins
        self.offset_centers = 0.5 * (offset_bins[1:] + offset_bins[:-1])
        self.min_counts = min_counts
        
        # 创建表格列表，每个区间对应一个表格
        # 区间数量 = len(offset_bins) - 1
        self.num_tables = len(offset_bins) - 1
        self.tables = []
        
        for i in range(self.num_tables):
            table = SigmaLookupTable(x_bins, y_bins, min_counts)
            table.offset_idx = i  # 区间索引
            table.offset_range = (offset_bins[i], offset_bins[i+1])  # 区间范围
            table.offset_center = self.offset_centers[i]  # 区间中心
            self.tables.append(table)
    
    def _get_offset_index(self, offset: float) -> int:
        """
        根据offset值获取对应的bin索引
        
        Parameters
        ----------
        offset : float
            Offset值
            
        Returns
        -------
        int
            对应的bin索引
        """
        # 使用 np.digitize 找到offset所在的bin
        # bins='right' 表示区间是左闭右开 [a, b)
        idx = np.digitize(offset, self.offset_bins, right=False) - 1
        
        # 处理边界情况
        if idx < 0:
            idx = 0
        elif idx >= self.num_tables:
            idx = self.num_tables - 1
            
        return idx
    
    def get_table_by_offset(self, offset: float) -> SigmaLookupTable:
        """
        根据offset值获取对应区间的表格
        
        Parameters
        ----------
        offset : float
            Offset值（连续值）
            
        Returns
        -------
        SigmaLookupTable
            对应区间的查找表
        """
        idx = self._get_offset_index(offset)
        return self.tables[idx]
    
    def fill(self, offset: np.ndarray, x: np.ndarray, y: np.ndarray, z: np.ndarray):
        """
        根据offset值将数据填充到对应的表格中
        
        Parameters
        ----------
        offset : np.ndarray
            Offset值数组（连续值）
        x, y, z : np.ndarray
            数据点坐标和值
        """
        # 确保所有数组长度相同
        assert len(offset) == len(x) == len(y) == len(z), \
            "All input arrays must have the same length"
        
        # 将数据按offset分配到不同的表格
        offset_indices = np.digitize(offset, self.offset_bins, right=False) - 1
        offset_indices = np.clip(offset_indices, 0, self.num_tables - 1)
        
        # 为每个表格填充对应的数据
        for i in range(self.num_tables):
            mask = offset_indices == i
            if np.sum(mask) > 0:
                self.tables[i].fill(x[mask], y[mask], z[mask])
    
    def smooth_table(self, table_idx: int, sigma: float = 2):
        """
        平滑指定索引的表格
        
        Parameters
        ----------
        table_idx : int
            表格索引 (0 到 num_tables-1)
        sigma : float
            高斯核标准差
        """
        if 0 <= table_idx < self.num_tables:
            self.tables[table_idx].smooth(sigma)
        else:
            raise IndexError(f"Table index {table_idx} out of range [0, {self.num_tables-1}]")
    
    def smooth_all(self, sigma: float = 2):
        """
        平滑所有表格
        
        Parameters
        ----------
        sigma : float
            高斯核标准差
        """
        for table in self.tables:
            if table.data_map is not None:
                table.smooth(sigma)
    
    def __call__(self, offset: float, x: float, y: float) -> float:
        """
        使用offset值选择对应表格进行插值查询
        
        Parameters
        ----------
        offset : float
            Offset值（连续值）
        x, y : float
            查询坐标
            
        Returns
        -------
        float
            插值结果
        """
        table = self.get_table_by_offset(offset)
        return table(x, y)
    
    def display(self, ax=None, original_data=False, log_scale=False, 
                show_all=True, selected_indices=None, show_counts=False, **kwargs):
        """
        显示表格数据
        
        Parameters
        ----------
        ax : matplotlib.axes.Axes, optional
            Axes to plot on. If None, creates new figure and axes.
        original_data : bool, optional
            If True, display the original data map. Default is False.
        log_scale : bool, optional
            If True, use logarithmic scale for the colorbar. Default is False.
        show_all : bool, optional
            If True, show all tables in subplots. Default is True.
        selected_indices : list, optional
            List of table indices to display. If None, show all.
        show_counts : bool, optional
            If True, display the counts map. Default is False.
        **kwargs
            Additional keyword arguments passed to imshow.
            
        Returns
        -------
        matplotlib.image.AxesImage or list
            The image object(s) returned by imshow.
        """
        import matplotlib.pyplot as plt
        
        # 确定要显示的表格索引
        if selected_indices is None:
            selected_indices = list(range(self.num_tables))
        else:
            selected_indices = [i for i in selected_indices if 0 <= i < self.num_tables]
        
        # 过滤掉没有数据的表格
        valid_indices = []
        for idx in selected_indices:
            table = self.tables[idx]
            if show_counts:
                data = table.counts_map
            elif original_data:
                data = table.data_map
            else:
                data = table.smoothed_data_map
            if data is not None and not np.all(np.isnan(data)):
                valid_indices.append(idx)
        
        if len(valid_indices) == 0:
            raise ValueError("No valid tables to display (all are None or all NaN)")
        
        if show_all and len(valid_indices) > 1:
            # 创建子图显示所有表格
            n_plots = len(valid_indices)
            cols = min(3, n_plots)  # 最多3列
            rows = (n_plots + cols - 1) // cols
            
            fig, axes = plt.subplots(rows, cols, figsize=(4*cols, 3*rows))
            if n_plots == 1:
                axes = [axes]
            elif rows == 1:
                axes = axes if isinstance(axes, np.ndarray) else [axes]
            else:
                axes = axes.flatten()
            
            images = []
            for plot_idx, table_idx in enumerate(valid_indices):
                table = self.tables[table_idx]
                ax_current = axes[plot_idx]
                
                try:
                    im = table.display(ax=ax_current, original_data=original_data, 
                                     log_scale=log_scale, show_counts=show_counts, **kwargs)
                    offset_range = table.offset_range
                    ax_current.set_title(f'Offset ∈ [{offset_range[0]:.1f}, {offset_range[1]:.1f})')
                    images.append(im)
                except (ValueError, RuntimeError) as e:
                    # 如果某个表格显示失败，显示错误信息
                    ax_current.text(0.5, 0.5, f'Error: {str(e)}', 
                           ha='center', va='center', transform=ax_current.transAxes)
                    ax_current.set_title(f'Offset ∈ [{table.offset_range[0]:.1f}, {table.offset_range[1]:.1f}) - Error')
            
            # 隐藏多余的子图
            for i in range(len(valid_indices), len(axes)):
                axes[i].set_visible(False)
            
            plt.tight_layout()
            return images
        else:
            # 显示单个表格
            table_idx = valid_indices[0]
            table = self.tables[table_idx]
            im = table.display(ax=ax, original_data=original_data, 
                             log_scale=log_scale, show_counts=show_counts, **kwargs)
            if ax is not None:
                offset_range = table.offset_range
                ax.set_title(f'Offset ∈ [{offset_range[0]:.1f}, {offset_range[1]:.1f})')
            return im
    def save(self, filename: str):
        """
        保存整个集合到文件
        
        Parameters
        ----------
        filename : str
            保存文件名
        """
        # 准备保存数据
        save_data = {
            'x_bins': self.x_bins,
            'y_bins': self.y_bins,
            'offset_bins': self.offset_bins,
            'min_counts': self.min_counts,
            'num_tables': self.num_tables
        }
        
        # 保存每个表格的数据
        for i, table in enumerate(self.tables):
            if table.smoothed_data_map is not None:
                save_data[f'table_{i}_data_map'] = table.data_map
                save_data[f'table_{i}_counts_map'] = table.counts_map
                save_data[f'table_{i}_smoothed_data_map'] = table.smoothed_data_map
        
        np.savez(filename, **save_data)
    
    @classmethod
    def load(cls, filename: str):
        """
        从文件加载集合
        
        Parameters
        ----------
        filename : str
            文件名
            
        Returns
        -------
        SigmaLookupTableCollection
            加载的集合对象
        """
        with np.load(filename) as data:
            # 创建集合对象
            collection = cls.__new__(cls)
            collection.x_bins = data['x_bins']
            collection.y_bins = data['y_bins']
            collection.offset_bins = data['offset_bins']
            collection.min_counts = data['min_counts']
            collection.num_tables = data['num_tables']
            collection.offset_centers = 0.5 * (collection.offset_bins[1:] + collection.offset_bins[:-1])
            
            # 重新创建表格
            collection.tables = []
            for i in range(collection.num_tables):
                table = SigmaLookupTable(collection.x_bins, collection.y_bins, collection.min_counts)
                table.offset_idx = i
                table.offset_range = (collection.offset_bins[i], collection.offset_bins[i+1])
                table.offset_center = collection.offset_centers[i]
                
                # 加载表格数据
                if f'table_{i}_data_map' in data:
                    table.data_map = data[f'table_{i}_data_map']
                    table.counts_map = data[f'table_{i}_counts_map']
                    table.smoothed_data_map = data[f'table_{i}_smoothed_data_map']
                    
                    # 重建插值器
                    table._interpolator = RegularGridInterpolator(
                        (table.x_centers, table.y_centers), 
                        table.smoothed_data_map, 
                        bounds_error=False, 
                        fill_value=None
                    )
                
                collection.tables.append(table)
        
        return collection
    
    def get_table_info(self, table_idx: int) -> dict:
        """
        获取指定索引表格的信息
        
        Parameters
        ----------
        table_idx : int
            表格索引
            
        Returns
        -------
        dict
            表格信息字典
        """
        if 0 <= table_idx < self.num_tables:
            table = self.tables[table_idx]
            return {
                'table_idx': table_idx,
                'offset_range': table.offset_range,
                'offset_center': table.offset_center,
                'has_data': table.data_map is not None,
                'has_smoothed': table.smoothed_data_map is not None,
                'min_counts': table.min_counts,
                'shape': table.data_map.shape if table.data_map is not None else None
            }
        else:
            raise IndexError(f"Table index {table_idx} out of range [0, {self.num_tables-1}]")
    
    def get_all_tables_info(self) -> list:
        """
        获取所有表格的信息
        
        Returns
        -------
        list
            所有表格信息列表
        """
        return [self.get_table_info(i) for i in range(self.num_tables)]