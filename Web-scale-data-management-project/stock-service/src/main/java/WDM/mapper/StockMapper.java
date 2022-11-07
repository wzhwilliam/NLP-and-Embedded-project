package WDM.mapper;

import WDM.pojo.Stock;
import org.apache.ibatis.annotations.Insert;
import org.apache.ibatis.annotations.Select;
import org.apache.ibatis.annotations.Update;

public interface StockMapper {
    @Select("select * from stock where itemid = #{itemId} ")
    Stock queryById(long itemId);

    @Update("update stock set amount = amount - #{num} where itemid = #{itemId}")
    Boolean subtract(long itemId, int num);

    @Update("update stock set amount = amount + #{num} where itemid = #{itemId}")
    Boolean add(long itemId, int num);

    @Insert("insert into stock(itemid, price) values(#{itemId}, #{price})")
    Boolean create(long itemId, double price);
}
