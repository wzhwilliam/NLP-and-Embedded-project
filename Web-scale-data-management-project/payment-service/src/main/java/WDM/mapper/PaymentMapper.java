package WDM.mapper;

import WDM.pojo.Payment;
import org.apache.ibatis.annotations.Insert;
import org.apache.ibatis.annotations.Select;
import org.apache.ibatis.annotations.Update;


public interface PaymentMapper {

    @Update("update payment set credit = credit - #{credit} where userid = #{userId}")
    Boolean pay(long userId, double credit);

    @Update("update payment set credit = credit + #{credit} where userid = #{userId}")
    Boolean add(long userId, double credit);

    @Insert("insert into payment(userid, credit) values(#{userId}, 0)")
    Boolean create(long userId);

    @Select("select * from payment where userid = #{userId}")
    Payment queryById(long userId);
}
