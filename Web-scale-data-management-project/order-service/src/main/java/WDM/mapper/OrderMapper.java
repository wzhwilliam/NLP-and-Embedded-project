package WDM.mapper;

import WDM.pojo.Order;
import org.apache.ibatis.annotations.Delete;
import org.apache.ibatis.annotations.Insert;
import org.apache.ibatis.annotations.Select;
import org.apache.ibatis.annotations.Update;


public interface OrderMapper {

    @Insert("insert into orderinfo(orderid, userid, paid) values(#{orderId}, #{userId}, false)")
    Boolean createOrder(long userId, long orderId);

    ///orders/remove/{order_id}
    //    DELETE - deletes an order by ID
    @Delete({"delete from orderinfo where orderid = #{orderId};", "delete from orderitem where orderid = #{orderId};"})
    Boolean removeOrder(long orderId);

    ///orders/find/{order_id}
    //    GET - retrieves the information of an order (id, payment status, items included and user id)
    //    Output JSON fields:
    //            “order_id”  - the order’s id
    //            “paid” (true/false)
    //            “items”  - list of item ids that are included in the order
    //            “user_id”  - the user’s id that made the order
    //            “total_cost” - the total cost of the items in the order
    @Select("select * from orderinfo where orderid = #{orderId}")
    Order findOrder(long orderId);


    ///orders/checkout/{order_id}
    // add one new column "amount" to orderitem to indicate purchase amount (makes comparison easier.)
    //    POST - makes the payment (via calling the payment service), subtracts the stock (via the stock service) and returns a status (success/failure).
    @Update("update orderinfo set paid = true where orderid = #{orderId}")
    void checkout(long orderId);

    @Update("update orderinfo set paid = false where orderid = #{orderId}")
    void cancelOrder(long orderId);
}
